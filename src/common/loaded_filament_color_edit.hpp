#pragma once

#include <loaded_filament_color.hpp>

namespace filament {

struct LoadedColorEditSnapshot {
    uint8_t material;
    uint64_t declaration;
};

// The caller checks the current head/material and operation state immediately
// before applying. transform() compares and writes the declaration under the
// journal lock, so a declaration changed while the palette was open is kept.
// A loaded material with no previous declaration can be declared without reload.
template <typename Declarations>
bool try_edit_loaded_color(Declarations &declarations, uint8_t slot,
    LoadedColorEditSnapshot expected, uint8_t current_material, bool available,
    std::optional<Color> color) {
    if (!available || !expected.material || current_material != expected.material) {
        return false;
    }

    bool accepted = false;
    declarations.transform(slot, [&](uint64_t current) {
        if (current != expected.declaration) {
            return current;
        }
        accepted = true;
        return encode_loaded_color(current_material, color);
    });
    return accepted;
}

} // namespace filament
