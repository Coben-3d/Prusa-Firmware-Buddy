#pragma once
#include <segmented_json.h>
#include <array>
#include <cstdint>

namespace nhttp::handler {
struct FilamentSlotState {
    std::array<char, 8> material {};
    uint64_t declaration = 0;
    uint8_t virtual_tool = 0;
    bool enabled = false;
    bool loaded = false;
};
// Captured once per request; the loop cursor survives chunked JSON resumption.
struct FilamentState {
    std::array<FilamentSlotState, 8> slots {};
    uint8_t index = 0;
};
class FilamentRenderer final : public json::JsonRenderer<FilamentState> {
public:
    explicit FilamentRenderer(FilamentState state) : JsonRenderer(state) {}
    json::JsonResult renderState(size_t, json::JsonOutput &, FilamentState &) const final;
};
}
