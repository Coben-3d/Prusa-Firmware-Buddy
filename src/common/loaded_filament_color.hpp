#pragma once

#include <color.hpp>

namespace filament {

// One journal item contains both the declared RGB and the material it describes.
// Zero means unconfirmed. A confirmed load may still have an unknown color.
// Do not change this encoding without migrating the journal key.
constexpr uint64_t encode_loaded_color(uint8_t material, std::optional<Color> color) {
    return material ? (uint64_t(material) << 32) | (color ? 0x01000000u | (color->raw & 0x00ffffffu) : 0u) : 0;
}

constexpr uint8_t loaded_color_material(uint64_t encoded) {
    return static_cast<uint8_t>(encoded >> 32);
}

constexpr std::optional<Color> decode_loaded_color(uint64_t encoded) {
    return loaded_color_material(encoded) && (encoded & 0x01000000u) ? std::optional(Color::from_raw(encoded & 0x00ffffffu)) : std::nullopt;
}

} // namespace filament
