#include "filament_renderer.h"
#include <loaded_filament_color.hpp>
#include <segmented_json_macros.h>

namespace nhttp::handler {
json::JsonResult FilamentRenderer::renderState(size_t resume_point, json::JsonOutput &output, FilamentState &state) const {
    JSON_START;
    JSON_OBJ_START;
    JSON_FIELD_INT("schema_version", 2); JSON_COMMA;
    JSON_FIELD_STR("printer_model", "COREONE_INDX"); JSON_COMMA;
    JSON_FIELD_STR("indexing", "physical_tools"); JSON_COMMA;
    JSON_FIELD_INT("tool_count", 8); JSON_COMMA;
    JSON_FIELD_ARR("slots");
    for (state.index = 0; state.index < state.slots.size(); ++state.index) {
        if (state.index) { JSON_COMMA; }
        JSON_OBJ_START;
        JSON_FIELD_INT("slot", state.index); JSON_COMMA;
        JSON_FIELD_INT("virtual_tool", state.slots[state.index].virtual_tool); JSON_COMMA;
        JSON_FIELD_BOOL("enabled", state.slots[state.index].enabled); JSON_COMMA;
        JSON_FIELD_BOOL("loaded", state.slots[state.index].loaded); JSON_COMMA;
        if (state.slots[state.index].material[0]) {
            JSON_FIELD_STR("material", state.slots[state.index].material.data());
        } else { JSON_CONTROL("\"material\":null"); }
        JSON_COMMA;
        if (filament::decode_loaded_color(state.slots[state.index].declaration)) {
            JSON_FIELD_STR_FORMAT("color", "#%06lX", static_cast<unsigned long>(state.slots[state.index].declaration & 0x00ffffffu));
        } else { JSON_CONTROL("\"color\":null"); }
        JSON_COMMA;
        JSON_FIELD_STR("source", "user_declared");
        JSON_OBJ_END;
    }
    JSON_ARR_END;
    JSON_OBJ_END;
    JSON_END;
}
}
