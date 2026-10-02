#include "filament_to_load.hpp"
#include <atomic>

static FilamentType filament_to_load = FilamentType::none;

FilamentType filament::get_type_to_load() {
    return filament_to_load;
}

void filament::set_type_to_load(FilamentType filament) {
    filament_to_load = filament;
}

// GUI selection and Marlin load execution share this one operation value.
// Every load entry point initializes it; no previous tool's color is inherited.
static std::atomic<uint32_t> color_to_load { 0 };

std::optional<Color> filament::get_color_to_load() {
    const auto encoded = color_to_load.load();
    return encoded & 0x01000000u ? std::optional(Color::from_raw(encoded & 0x00ffffffu)) : std::nullopt;
}

void filament::set_color_to_load(std::optional<Color> color) {
    color_to_load.store(color ? 0x01000000u | (color->raw & 0x00ffffffu) : 0u);
}
