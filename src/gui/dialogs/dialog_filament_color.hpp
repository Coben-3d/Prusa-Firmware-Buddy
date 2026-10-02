#pragma once
#include <filament_color_palette.hpp>

// CORE One INDX / 480x320. Accepting Unknown is distinct from cancelling.
filament::ColorPaletteResult select_filament_color_dialog(std::optional<Color> initial);
