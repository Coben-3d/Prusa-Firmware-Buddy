#pragma once

#include <utils/color.hpp>
#include <array>
#include <algorithm>
#include <optional>

namespace filament {

// Manually chosen display colors, not manufacturer data or measured spool colors.
// Each row offers six increasingly light shades. Keep the original palette RGBs.
inline constexpr std::array<uint32_t, 60> color_palette_rgb {
    0x000000, 0x333333, 0x5b5b5b, 0x808080, 0xc0c0c0, 0xffffff,
    0x3e2415, 0x633a20, 0x8b4513, 0xab7046, 0xd2a679, 0xead2b4,
    0x000080, 0x0000ff, 0x1e5a96, 0x3b82f6, 0x80b5ed, 0xc1dcff,
    0x003b20, 0x008000, 0x348a42, 0x65b86e, 0xa3d9a5, 0xd5eed5,
    0x660000, 0xa00000, 0xff0000, 0xee6666, 0xf4a0a0, 0xfad6d6,
    0x71300a, 0xaf4b10, 0xf8651b, 0xff9550, 0xffbe87, 0xffdfbd,
    0x665000, 0xaa8800, 0xe0bc00, 0xffff00, 0xffef80, 0xfff7bf,
    0x330044, 0x800080, 0x8d4bb0, 0xb47ad0, 0xd5afe5, 0xedd8f4,
    0x6b163d, 0xad3265, 0xff69b4, 0xf58eba, 0xf8bed7, 0xfbe0eb,
    0x004044, 0x008080, 0x199da6, 0x62bdc5, 0xa1dce0, 0xd5eff0,
};

struct ColorPaletteResult {
    // Unknown is an accepted nullopt. Cancel is not an accepted selection.
    bool accepted = false;
    std::optional<Color> color;
};

// UI-independent state, also used by native tests. Does not change pending or
// persisted filament declarations; the caller commits only an accepted result.
class ColorPaletteModel {
public:
    static constexpr int columns = 6;
    static constexpr int rows = 4;
    static constexpr int page_size = columns * rows;
    static constexpr int page_count = (color_palette_rgb.size() + page_size - 1) / page_size;
    static constexpr int back = page_size;
    static constexpr int unknown = back + 1;
    static constexpr int previous_page = back + 2;
    static constexpr int next_page = back + 3;

    explicit ColorPaletteModel(std::optional<Color> initial)
        : initial_(initial), result_ { false, initial } {
        if (!initial) {
            focus_ = unknown;
            return;
        }
        for (size_t i = 0; i < color_palette_rgb.size(); ++i) {
            if (initial->raw == color_palette_rgb[i]) {
                page_ = i / page_size;
                focus_ = i % page_size;
                return;
            }
        }
        // Preserve arbitrary existing RGBs. Opening/returning never snaps them
        // to the nearest palette entry.
        focus_ = back;
    }

    int page() const { return page_; }
    int focus() const { return focus_; }
    int color_count() const {
        return std::min(page_size, static_cast<int>(color_palette_rgb.size()) - page_ * page_size);
    }
    Color color_at(int index) const {
        return Color::from_raw(color_palette_rgb[page_ * page_size + index]);
    }
    std::optional<Color> initial() const { return initial_; }
    ColorPaletteResult result() const { return result_; }
    std::optional<Color> preview() const {
        if (focus_ < color_count()) return color_at(focus_);
        if (focus_ == unknown) return std::nullopt;
        return initial_;
    }
    bool set_focus(int index) {
        if (index < 0 || (index >= color_count() && (index < back || index > next_page))) return false;
        focus_ = index;
        return true;
    }
    bool move(int diff) {
        const int previous = focus_;
        const int count = color_count();
        const int linear = focus_ < count ? focus_ : count + focus_ - back;
        // Saturate before adding, including extreme encoder deltas.
        const int next = std::clamp(linear + std::clamp(diff, -page_size - 4, page_size + 4), 0, count + 3);
        focus_ = next < count ? next : back + next - count;
        return previous != focus_;
    }
    // Returns true when the dialog should close. Browsing pages is not a commit.
    bool activate() {
        if (focus_ == back) return true;
        if (focus_ == previous_page || focus_ == next_page) {
            page_ = (page_ + (focus_ == next_page ? 1 : page_count - 1)) % page_count;
            return false;
        }
        result_ = { true, focus_ == unknown ? std::nullopt : std::optional(color_at(focus_)) };
        return true;
    }

private:
    std::optional<Color> initial_;
    ColorPaletteResult result_;
    int page_ = 0;
    int focus_ = unknown;
};

} // namespace filament
