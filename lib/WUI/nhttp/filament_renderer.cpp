#include "filament_renderer.h"

#include <loaded_filament_color.hpp>
#include <segmented_json_macros.h>

namespace nhttp::handler {

json::JsonResult FilamentRenderer::renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const {
    const auto color = filament::decode_loaded_color(state.declaration);

    JSON_START;
    JSON_OBJ_START;
    JSON_FIELD_INT("schema_version", 1);
    JSON_COMMA;
    JSON_FIELD_ARR("slots");
    JSON_OBJ_START;
    JSON_FIELD_INT("slot", 0);
    JSON_COMMA;
    if (state.material[0]) {
        JSON_FIELD_STR("material", state.material.data());
    } else {
        JSON_CONTROL("\"material\":null");
    }
    JSON_COMMA;
    if (color) {
        JSON_FIELD_STR_FORMAT("color", "#%06lX", static_cast<unsigned long>(color->raw));
    } else {
        JSON_CONTROL("\"color\":null");
    }
    JSON_COMMA;
    JSON_FIELD_STR("source", "user_declared");
    JSON_OBJ_END;
    JSON_ARR_END;
    JSON_OBJ_END;
    JSON_END;
}

} // namespace nhttp::handler
