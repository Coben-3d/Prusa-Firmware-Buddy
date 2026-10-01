#pragma once

#include <segmented_json.h>
#include <array>
#include <cstdint>

namespace nhttp::handler {

// Captured once per request, including across chunked JSON resumption.
struct FilamentState {
    std::array<char, 8> material {};
    uint64_t declaration = 0;
};

class FilamentRenderer final : public json::JsonRenderer<FilamentState> {
public:
    FilamentRenderer(FilamentState state)
        : JsonRenderer(state) {}
    json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const final;
};

} // namespace nhttp::handler
