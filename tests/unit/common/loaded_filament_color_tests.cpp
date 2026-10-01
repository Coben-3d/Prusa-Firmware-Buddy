#include <catch2/catch.hpp>
#include <loaded_filament_color.hpp>
#include <filament_to_load.hpp>
#include <filament_renderer.h>
#include <journal/backend.hpp>

#include <array>
#include <cstring>
#include <memory>
#include <thread>

namespace {
constexpr uint16_t test_id = 123;

class MemoryStorage final : public configuration_store::Storage {
public:
    std::array<uint8_t, 2048> bytes;
    std::optional<size_t> write_budget;

    MemoryStorage() { bytes.fill(0xff); }
    uint8_t read_byte(uint16_t address) final { return bytes.at(address); }
    void read_bytes(uint16_t address, std::span<uint8_t> data) final { std::copy_n(bytes.begin() + address, data.size(), data.begin()); }
    void write_byte(uint16_t address, uint8_t value) final {
        if (write_budget) {
            if (!*write_budget) {
                return;
            }
            --*write_budget;
        }
        bytes.at(address) = value;
    }
    void write_bytes(uint16_t address, std::span<const uint8_t> data) final {
        for (const auto value : data) {
            write_byte(address++, value);
        }
    }
    void erase_area(uint16_t begin, uint16_t end) final { std::fill(bytes.begin() + begin, bytes.begin() + end, 0xff); }
};

class JournalRecord {
    uint64_t value_ = 0;
    journal::Backend backend;

public:
    JournalRecord(MemoryStorage &storage)
        : backend(0, storage.bytes.size(), storage) {
        backend.init([this] {
            if (value_) {
                backend.save(test_id, { reinterpret_cast<const uint8_t *>(&value_), sizeof(value_) });
            }
        });
        backend.load_all([this](uint16_t id, const std::span<const uint8_t> &bytes) {
            if (id == test_id) {
                REQUIRE(bytes.size() == sizeof(value_));
                memcpy(&value_, bytes.data(), sizeof(value_));
            }
        },
            {});
    }
    void set(uint64_t value) {
        value_ = value;
        backend.save(test_id, { reinterpret_cast<const uint8_t *>(&value_), sizeof(value_) });
    }
    uint64_t get() const { return value_; }
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

TEST_CASE("Local filament API renders stable JSON in small chunks", "[loaded-color]") {
    nhttp::handler::FilamentState state;
    memcpy(state.material.data(), "PLA", 4);
    state.declaration = filament::encode_loaded_color(1, COLOR_BLACK);
    const std::string expected = R"({"schema_version":1,"slots":[{"slot":0,"material":"PLA","color":"#000000","source":"user_declared"}]})";
    for (size_t chunk = 32; chunk <= 256; ++chunk) {
        REQUIRE(render(state, chunk) == expected);
    }
    state.declaration = 0;
    REQUIRE(render(state, 32).find(R"("color":null)") != std::string::npos);
    state.material.fill(0);
    REQUIRE(render(state, 32).find(R"("material":null)") != std::string::npos);
    memcpy(state.material.data(), "P\"LA", 5);
    REQUIRE(render(state, 64).find(R"("material":"P\"LA")") != std::string::npos);
}

TEST_CASE("HTTP JSON renderer owns its snapshot", "[loaded-color]") {
    nhttp::handler::FilamentState state;
    memcpy(state.material.data(), "PETG", 5);
    state.declaration = filament::encode_loaded_color(2, COLOR_RED);
    nhttp::handler::FilamentRenderer renderer(state);
    state.declaration = 0;
    state.material.fill(0);
    std::array<uint8_t, 256> buffer;
    const auto [status, length] = renderer.render(buffer.data(), buffer.size());
    REQUIRE(status == json::JsonResult::Complete);
    const std::string result(reinterpret_cast<char *>(buffer.data()), length);
    REQUIRE(result.find("PETG") != std::string::npos);
    REQUIRE(result.find("#FF0000") != std::string::npos);
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
