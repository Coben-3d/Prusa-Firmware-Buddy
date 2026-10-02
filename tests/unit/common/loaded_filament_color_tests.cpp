#include <catch2/catch_test_macros.hpp>
#include <loaded_filament_color.hpp>
#include <loaded_filament_color_edit.hpp>
#include <journal/store_item.hpp>
#include <journal/store_item_array.hpp>
#include <filament_to_load.hpp>
#include <filament_color_palette.hpp>
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


TEST_CASE("Three brown and three blue palette shades keep exact RGB per physical head", "[loaded-color]") {
    auto state = eight_tools();
    constexpr std::array<size_t, 6> palette_indices { 6, 8, 10, 12, 15, 17 };
    MemoryStorage storage;
    {
        JournalRecord record(storage);
        for (size_t i = 0; i < palette_indices.size(); ++i) {
            const auto color = Color::from_raw(filament::color_palette_rgb[palette_indices[i]]);
            filament::ColorPaletteModel model(color);
            REQUIRE(model.activate());
            REQUIRE(model.result().accepted);
            REQUIRE(model.result().color == color);
            const auto declaration = filament::encode_loaded_color(i % 2 ? 2 : 1, model.result().color);
            record.set(declaration, i);
            state.slots[i].declaration = declaration;
        }
    }
    JournalRecord rebooted(storage);
    const auto body = render(state, 64);
    if (const char *path = std::getenv("INDX_SHADE_FIXTURE")) std::ofstream(path) << body;
    for (size_t i = 0; i < palette_indices.size(); ++i) {
        const auto color = Color::from_raw(filament::color_palette_rgb[palette_indices[i]]);
        REQUIRE(filament::decode_loaded_color(rebooted.get(i)) == color);
        char hex[8];
        snprintf(hex, sizeof(hex), "#%06lX", static_cast<unsigned long>(color.raw));
        REQUIRE(body.find(hex) != std::string::npos);
    }
}


namespace {
journal::Backend *editing_backend = nullptr;
journal::Backend &get_editing_backend() { return *editing_backend; }
using EditingColors = journal::JournalItemArray<uint64_t, 0, 0, get_editing_backend, test_id, 16, 8>;

class EditingJournal {
    journal::Backend backend_;

public:
    EditingColors colors {};

    explicit EditingJournal(MemoryStorage &storage)
        : backend_(0, storage.bytes.size(), storage) {
        REQUIRE(editing_backend == nullptr);
        editing_backend = &backend_;
        backend_.init([this] { colors.ram_dump(0); });
        backend_.load_all([this](uint16_t id, const Bytes &data) {
            colors.check_init(id, data);
        }, {});
    }
    ~EditingJournal() { editing_backend = nullptr; }
};
} // namespace

TEST_CASE("Direct correction keeps eight head materials, other colors and pending load separate", "[loaded-color][color-edit]") {
    for (uint8_t target = 0; target < 8; ++target) {
        MemoryStorage storage;
        auto state = eight_tools();
        std::array<uint64_t, 8> expected;
        filament::set_color_to_load(COLOR_RED);
        {
            EditingJournal journal(storage);
            for (uint8_t i = 0; i < 8; ++i) {
                expected[i] = state.slots[i].declaration;
                journal.colors.set(i, expected[i]);
            }
            const auto material = filament::loaded_color_material(expected[target]);
            const filament::LoadedColorEditSnapshot before { material, expected[target] };
            const auto chosen = Color::from_raw(filament::color_palette_rgb[6 + target]);
            REQUIRE(filament::try_edit_loaded_color(journal.colors, target, before, material, true, chosen));
            expected[target] = filament::encode_loaded_color(material, chosen);
            REQUIRE(filament::get_color_to_load() == COLOR_RED);
            REQUIRE(filament::loaded_color_material(journal.colors.get(target)) == material);
            const auto bytes_after = storage.bytes;
            // Reconfirming the same choice does not write another journal record.
            REQUIRE(filament::try_edit_loaded_color(journal.colors, target,
                { material, expected[target] }, material, true, chosen));
            REQUIRE(storage.bytes == bytes_after);
        }
        EditingJournal rebooted(storage);
        for (uint8_t i = 0; i < 8; ++i) {
            REQUIRE(rebooted.colors.get(i) == expected[i]);
            state.slots[i].declaration = rebooted.colors.get(i);
        }
        const auto body = render(state, 64);
        for (uint8_t i = 0; i < 8; ++i) {
            char hex[8];
            snprintf(hex, sizeof(hex), "#%06lX", static_cast<unsigned long>(filament::decode_loaded_color(expected[i])->raw));
            REQUIRE(body.find(hex) != std::string::npos);
        }
        REQUIRE(body.find("PLA") != std::string::npos);
        REQUIRE(body.find("PETG") != std::string::npos);
        if (target == 7) {
            if (const char *fixture = std::getenv("INDX_EDIT_FIXTURE")) std::ofstream(fixture) << body;
        }
    }
    filament::set_color_to_load(std::nullopt);
}

TEST_CASE("Empty or busy heads and changed material or declaration reject a stale edit", "[loaded-color][color-edit]") {
    MemoryStorage storage;
    EditingJournal journal(storage);
    const auto original = filament::encode_loaded_color(1, COLOR_BLUE);
    journal.colors.set(3, original);
    const filament::LoadedColorEditSnapshot before { 1, original };
    const auto bytes_before = storage.bytes;
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, before, 1, false, COLOR_WHITE));
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, before, 0, true, COLOR_WHITE));
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, before, 2, true, COLOR_WHITE));
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, { 0, original }, 0, true, COLOR_WHITE));
    REQUIRE(storage.bytes == bytes_before);
    journal.colors.set(3, 0); // Unload/load start invalidated the declaration.
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, before, 1, true, COLOR_WHITE));
    REQUIRE(journal.colors.get(3) == 0);
    const auto newer = filament::encode_loaded_color(1, COLOR_PURPLE);
    journal.colors.set(3, newer); // A newer confirmation must not be overwritten.
    REQUIRE_FALSE(filament::try_edit_loaded_color(journal.colors, 3, before, 1, true, COLOR_WHITE));
    REQUIRE(journal.colors.get(3) == newer);
}

TEST_CASE("Loaded material can be declared without reload and unknown remains different from black", "[loaded-color][color-edit]") {
    MemoryStorage storage;
    {
        EditingJournal journal(storage);
        REQUIRE(journal.colors.get(7) == 0);
        REQUIRE(filament::try_edit_loaded_color(journal.colors, 7, { 1, 0 }, 1, true, COLOR_BLACK));
        const auto black = journal.colors.get(7);
        REQUIRE(filament::decode_loaded_color(black) == COLOR_BLACK);
        filament::ColorPaletteModel cancelled(COLOR_BLACK);
        cancelled.move(3);
        REQUIRE_FALSE(cancelled.result().accepted);
        REQUIRE(journal.colors.get(7) == black);
        REQUIRE(filament::try_edit_loaded_color(journal.colors, 7, { 1, black }, 1, true, std::nullopt));
        REQUIRE(filament::loaded_color_material(journal.colors.get(7)) == 1);
    }
    EditingJournal rebooted(storage);
    REQUIRE_FALSE(filament::decode_loaded_color(rebooted.colors.get(7)));
    REQUIRE(filament::loaded_color_material(rebooted.colors.get(7)) == 1);
}

TEST_CASE("Interrupted direct correction leaves only the old or new color and preserves other heads", "[loaded-color][color-edit]") {
    for (size_t budget = 0; budget < 25; ++budget) {
        MemoryStorage storage;
        const auto old_color = filament::encode_loaded_color(1, COLOR_BLUE);
        const auto new_color = filament::encode_loaded_color(1, COLOR_WHITE);
        {
            EditingJournal journal(storage);
            for (uint8_t i = 0; i < 8; ++i) journal.colors.set(i, old_color);
            storage.write_budget = budget;
            REQUIRE(filament::try_edit_loaded_color(journal.colors, 5, { 1, old_color }, 1, true, COLOR_WHITE));
        }
        storage.write_budget.reset();
        EditingJournal rebooted(storage);
        for (uint8_t i = 0; i < 8; ++i) {
            if (i == 5) {
                REQUIRE((rebooted.colors.get(i) == old_color || rebooted.colors.get(i) == new_color));
            } else {
                REQUIRE(rebooted.colors.get(i) == old_color);
            }
        }
    }
}
