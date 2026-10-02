#include <catch2/catch_test_macros.hpp>
#include <loaded_filament_color.hpp>
#include <filament_to_load.hpp>
#include <filament_renderer.h>
#include <journal/backend.hpp>

#include <array>
#include <cstring>
#include <memory>
#include <thread>
#include <fstream>
#include <cstdlib>

namespace {
constexpr uint16_t test_id = 123;

class MemoryStorage final : public configuration_store::Storage {
public:
    std::array<std::byte, 2048> bytes;
    std::optional<size_t> write_budget;
    MemoryStorage() { bytes.fill(std::byte { 0xff }); }
    size_t read_bytes(size_t address, WritableBytes data) final {
        std::copy_n(bytes.begin() + address, data.size(), data.begin());
        return data.size();
    }
    size_t write_bytes(size_t address, Bytes data) final {
        size_t written = 0;
        for (const auto value : data) {
            if (write_budget && !*write_budget) break;
            if (write_budget) --*write_budget;
            bytes.at(address++) = value;
            ++written;
        }
        return written;
    }
    void flush() final {}
};

class JournalRecord {
    std::array<uint64_t, 8> values_ {};
    journal::Backend backend;
public:
    JournalRecord(MemoryStorage &storage) : backend(0, storage.bytes.size(), storage) {
        backend.init([this] {
            for (size_t i = 0; i < values_.size(); ++i) {
                if (values_[i]) {
                    backend.save(test_id + i, { reinterpret_cast<const std::byte *>(&values_[i]), sizeof(uint64_t) });
                }
            }
        });
        backend.load_all([this](uint16_t id, const Bytes &bytes) {
            if (id >= test_id && id < test_id + values_.size()) {
                REQUIRE(bytes.size() == sizeof(uint64_t));
                memcpy(&values_[id - test_id], bytes.data(), sizeof(uint64_t));
            }
        }, {});
    }
    void set(uint64_t value, size_t slot = 0) {
        values_.at(slot) = value;
        backend.save(test_id + slot, { reinterpret_cast<const std::byte *>(&values_[slot]), sizeof(uint64_t) });
    }
    uint64_t get(size_t slot = 0) const { return values_.at(slot); }
};

std::string render(nhttp::handler::FilamentState state, size_t chunk_size) {
    nhttp::handler::FilamentRenderer renderer(state);
    std::array<uint8_t, 256> buffer;
    std::string result;
    for (unsigned calls = 0; calls < 100; ++calls) {
        auto [status, length] = renderer.render(buffer.data(), chunk_size);
        result.append(reinterpret_cast<char *>(buffer.data()), length);
        if (status == json::JsonResult::Complete) {
            return result;
        }
        REQUIRE(status == json::JsonResult::Incomplete);
    }
    FAIL("JSON renderer made no progress");
    return {};
}
} // namespace

TEST_CASE("Loaded color keeps black, white, unknown and material tags distinct", "[loaded-color]") {
    for (const auto color : { COLOR_BLACK, COLOR_WHITE, COLOR_ORANGE, Color::from_raw(0x123456) }) {
        const auto record = filament::encode_loaded_color(1, color);
        REQUIRE(filament::loaded_color_material(record) == 1);
        REQUIRE(filament::decode_loaded_color(record) == color);
    }
    REQUIRE_FALSE(filament::decode_loaded_color(0));
    REQUIRE_FALSE(filament::decode_loaded_color(filament::encode_loaded_color(1, std::nullopt)));
    REQUIRE(filament::encode_loaded_color(0, COLOR_RED) == 0);
    REQUIRE(filament::decode_loaded_color(filament::encode_loaded_color(255, COLOR_BLACK)) == COLOR_BLACK);
}

TEST_CASE("GUI pending color is separate from persisted color and can be reset", "[loaded-color]") {
    filament::set_color_to_load(std::nullopt);
    REQUIRE_FALSE(filament::get_color_to_load());
    filament::set_color_to_load(COLOR_BLACK);
    REQUIRE(filament::get_color_to_load() == COLOR_BLACK);
    filament::set_color_to_load(COLOR_WHITE);
    REQUIRE(filament::get_color_to_load() == COLOR_WHITE);
    filament::set_color_to_load(std::nullopt);
    REQUIRE_FALSE(filament::get_color_to_load());
}

TEST_CASE("Pending GUI color is an untorn cross-task snapshot", "[loaded-color]") {
    filament::set_color_to_load(COLOR_BLACK);
    std::thread writer([] {
        for (unsigned i = 0; i < 100000; ++i) {
            filament::set_color_to_load(i % 2 ? COLOR_WHITE : COLOR_BLACK);
        }
    });
    bool torn = false;
    for (unsigned i = 0; i < 100000; ++i) {
        const auto color = filament::get_color_to_load();
        torn |= color != COLOR_WHITE && color != COLOR_BLACK;
    }
    writer.join();
    REQUIRE_FALSE(torn);
}

TEST_CASE("Color declaration survives journal reboot and invalidation", "[loaded-color]") {
    MemoryStorage storage;
    const auto black = filament::encode_loaded_color(1, COLOR_BLACK);
    {
        JournalRecord record(storage);
        REQUIRE(record.get() == 0); // Existing firmware has no entry.
        record.set(black);
    }
    {
        JournalRecord rebooted(storage);
        REQUIRE(rebooted.get() == black);
        // Selecting another pending color and then cancelling the preheat
        // menu does not touch the confirmed journal item.
        filament::set_color_to_load(COLOR_RED);
        REQUIRE(rebooted.get() == black);
        rebooted.set(0); // Load start, unload or sensor removal invalidates it.
    }
    {
        JournalRecord rebooted(storage);
        REQUIRE(rebooted.get() == 0);
        rebooted.set(filament::encode_loaded_color(1, std::nullopt));
    }
    JournalRecord rebooted(storage);
    REQUIRE(filament::loaded_color_material(rebooted.get()) == 1);
    REQUIRE_FALSE(filament::decode_loaded_color(rebooted.get()));
}


namespace {
nhttp::handler::FilamentState eight_tools() {
    nhttp::handler::FilamentState state;
    constexpr std::array<uint32_t, 8> colors { 0, 0xffffff, 0xff0000, 0x00ff00, 0xffff00, 0x00ffff, 0x800080, 0x123456 };
    for (size_t i = 0; i < state.slots.size(); ++i) {
        auto &slot = state.slots[i];
        slot.virtual_tool = i;
        slot.enabled = slot.loaded = true;
        memcpy(slot.material.data(), i % 2 ? "PETG" : "PLA", i % 2 ? 5 : 4);
        slot.declaration = filament::encode_loaded_color(i % 2 ? 2 : 1, Color::from_raw(colors[i]));
    }
    return state;
}
}

TEST_CASE("Eight local filament slots render in stable order in small chunks", "[loaded-color]") {
    auto state = eight_tools();
    const auto expected = render(state, 256);
    if (const char *path = std::getenv("INDX_RENDER_FIXTURE")) std::ofstream(path) << expected;
    REQUIRE(expected.find(R"("schema_version":2)") != std::string::npos);
    REQUIRE(expected.find(R"("printer_model":"COREONE_INDX")") != std::string::npos);
    REQUIRE(expected.find(R"("indexing":"physical_tools")") != std::string::npos);
    REQUIRE(expected.find(R"("tool_count":8)") != std::string::npos);
    size_t previous = 0;
    for (size_t i = 0; i < 8; ++i) {
        const auto index = expected.find("\"slot\":" + std::to_string(i));
        REQUIRE(index != std::string::npos);
        REQUIRE(index >= previous);
        previous = index;
    }
    for (size_t chunk = 64; chunk <= 256; ++chunk) REQUIRE(render(state, chunk) == expected);
    REQUIRE(expected.find("#000000") != std::string::npos);
    REQUIRE(expected.find("#800080") != std::string::npos);
    state.slots[3].enabled = state.slots[3].loaded = false;
    state.slots[3].material.fill(0);
    state.slots[3].declaration = 0;
    REQUIRE(render(state, 64).find(R"("slot":3,"virtual_tool":3,"enabled":false,"loaded":false,"material":null,"color":null)") != std::string::npos);
    memcpy(state.slots[1].material.data(), "P\"LA", 5);
    REQUIRE(render(state, 64).find(R"("material":"P\"LA")") != std::string::npos);
}

TEST_CASE("HTTP renderer owns an eight-tool snapshot", "[loaded-color]") {
    auto state = eight_tools();
    const auto expected = render(state, 64);
    nhttp::handler::FilamentRenderer renderer(state);
    state.slots = {};
    std::array<uint8_t, 128> buffer;
    std::string result;
    for (unsigned calls = 0; calls < 100; ++calls) {
        const auto [status, length] = renderer.render(buffer.data(), buffer.size());
        result.append(reinterpret_cast<char *>(buffer.data()), length);
        if (status == json::JsonResult::Complete) break;
        REQUIRE(status == json::JsonResult::Incomplete);
    }
    REQUIRE(result == expected);
}

TEST_CASE("Eight declarations survive reboot independently and journal compaction", "[loaded-color]") {
    MemoryStorage storage;
    std::array<uint64_t, 8> expected;
    {
        JournalRecord record(storage);
        for (size_t i = 0; i < 8; ++i) {
            expected[i] = filament::encode_loaded_color(i + 1, Color::from_raw(0x10101 * i));
            record.set(expected[i], i);
        }
        for (unsigned changes = 0; changes < 200; ++changes) {
            expected[3] = filament::encode_loaded_color(4, changes % 2 ? COLOR_WHITE : COLOR_BLACK);
            record.set(expected[3], 3);
        }
    }
    {
        JournalRecord rebooted(storage);
        for (size_t i = 0; i < 8; ++i) REQUIRE(rebooted.get(i) == expected[i]);
        rebooted.set(0, 6);
        expected[6] = 0;
    }
    JournalRecord rebooted(storage);
    for (size_t i = 0; i < 8; ++i) REQUIRE(rebooted.get(i) == expected[i]);
}

TEST_CASE("Interrupted confirmation on one head preserves all other heads", "[loaded-color]") {
    for (size_t written = 0; written < 25; ++written) {
        MemoryStorage storage;
        const auto other = filament::encode_loaded_color(2, COLOR_PURPLE);
        {
            JournalRecord record(storage);
            for (size_t i = 0; i < 8; ++i) record.set(other, i);
            record.set(0, 5);
            storage.write_budget = written;
            record.set(filament::encode_loaded_color(1, COLOR_WHITE), 5);
        }
        storage.write_budget.reset();
        JournalRecord rebooted(storage);
        for (size_t i = 0; i < 8; ++i) {
            if (i != 5) REQUIRE(rebooted.get(i) == other);
        }
        const auto color = filament::decode_loaded_color(rebooted.get(5));
        REQUIRE((!color || color == COLOR_WHITE));
        REQUIRE(color != COLOR_PURPLE);
    }
}

TEST_CASE("Interrupted confirmation cannot resurrect a previous spool color", "[loaded-color]") {
    // Simulate a power cut at each byte of the new confirmation transaction.
    for (size_t written = 0; written < 15; ++written) {
        MemoryStorage storage;
        {
            JournalRecord record(storage);
            record.set(filament::encode_loaded_color(1, COLOR_RED));
            record.set(0); // Persisted before the new load can start.
            storage.write_budget = written;
            record.set(filament::encode_loaded_color(1, COLOR_WHITE));
        }
        storage.write_budget.reset();
        JournalRecord rebooted(storage);
        const auto color = filament::decode_loaded_color(rebooted.get());
        REQUIRE((!color || color == COLOR_WHITE));
        REQUIRE(color != COLOR_RED);
    }
}
