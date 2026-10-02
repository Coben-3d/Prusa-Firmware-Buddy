#include <catch2/catch_test_macros.hpp>
#include <filament_color_palette.hpp>
#include <set>
#include <climits>

using filament::ColorPaletteModel;

TEST_CASE("Every palette shade can be selected independently without snapping", "[color-palette]") {
    std::set<uint32_t> colors;
    for (const auto rgb : filament::color_palette_rgb) {
        REQUIRE(rgb <= 0xffffff);
        REQUIRE(colors.insert(rgb).second);
        ColorPaletteModel model(Color::from_raw(rgb));
        REQUIRE(model.preview() == Color::from_raw(rgb));
        REQUIRE_FALSE(model.result().accepted);
        REQUIRE(model.activate());
        REQUIRE(model.result().accepted);
        REQUIRE(model.result().color == Color::from_raw(rgb));
    }
    // The previous palette remains representable, including known black.
    for (const auto color : { COLOR_BLACK, COLOR_WHITE, COLOR_GRAY, COLOR_RED, COLOR_ORANGE,
             COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_PURPLE, Color::from_raw(0x8b4513), Color::from_raw(0xff69b4) }) {
        REQUIRE(colors.count(color.raw) == 1);
    }
    // Row order is actually dark-to-light; RGB666 display conversion must not
    // collapse adjacent shades into an identical displayed swatch.
    for (size_t i = 1; i < filament::color_palette_rgb.size(); ++i) {
        if (i % ColorPaletteModel::columns == 0) continue;
        const auto previous = Color::from_raw(filament::color_palette_rgb[i - 1]);
        const auto current = Color::from_raw(filament::color_palette_rgb[i]);
        REQUIRE(current.to_grayscale() > previous.to_grayscale());
        REQUIRE((current.raw & 0xfcfcfc) != (previous.raw & 0xfcfcfc));
    }
}

TEST_CASE("Returning or browsing preserves arbitrary existing RGB and pending state", "[color-palette]") {
    const auto custom = Color::from_raw(0x123456);
    ColorPaletteModel model(custom);
    REQUIRE(model.focus() == ColorPaletteModel::back);
    REQUIRE(model.preview() == custom);
    REQUIRE(model.set_focus(ColorPaletteModel::next_page));
    for (int i = 0; i < ColorPaletteModel::page_count * 3; ++i) {
        REQUIRE_FALSE(model.activate());
        REQUIRE_FALSE(model.result().accepted);
        REQUIRE(model.result().color == custom);
    }
    REQUIRE(model.set_focus(0));
    REQUIRE(model.preview() != custom);
    REQUIRE_FALSE(model.result().accepted);
    REQUIRE(model.set_focus(ColorPaletteModel::back));
    REQUIRE(model.activate());
    REQUIRE_FALSE(model.result().accepted);
    REQUIRE(model.result().color == custom);
}

TEST_CASE("Unknown selection differs from cancellation and known black", "[color-palette]") {
    ColorPaletteModel unknown(COLOR_BLACK);
    REQUIRE(unknown.set_focus(ColorPaletteModel::unknown));
    REQUIRE_FALSE(unknown.preview());
    REQUIRE(unknown.activate());
    REQUIRE(unknown.result().accepted);
    REQUIRE_FALSE(unknown.result().color);

    ColorPaletteModel black(std::nullopt);
    REQUIRE(black.set_focus(0));
    REQUIRE(black.activate());
    REQUIRE(black.result().accepted);
    REQUIRE(black.result().color == COLOR_BLACK);

    ColorPaletteModel cancelled(std::nullopt);
    REQUIRE(cancelled.set_focus(ColorPaletteModel::back));
    REQUIRE(cancelled.activate());
    REQUIRE_FALSE(cancelled.result().accepted);
    REQUIRE_FALSE(cancelled.result().color);
}

TEST_CASE("Encoder and page changes skip unavailable cells and bound extreme deltas", "[color-palette]") {
    ColorPaletteModel model(COLOR_BLACK);
    REQUIRE(model.set_focus(ColorPaletteModel::previous_page));
    REQUIRE_FALSE(model.activate());
    REQUIRE(model.page() == ColorPaletteModel::page_count - 1);
    REQUIRE(model.color_count() == 12);
    for (int i = model.color_count(); i < ColorPaletteModel::page_size; ++i) REQUIRE_FALSE(model.set_focus(i));
    REQUIRE_FALSE(model.set_focus(-1));
    REQUIRE_FALSE(model.set_focus(100));
    REQUIRE(model.set_focus(model.color_count() - 1));
    REQUIRE(model.move(1));
    REQUIRE(model.focus() == ColorPaletteModel::back);
    REQUIRE(model.move(-1));
    REQUIRE(model.focus() == model.color_count() - 1);
    REQUIRE(model.move(INT_MAX));
    REQUIRE(model.focus() == ColorPaletteModel::next_page);
    REQUIRE_FALSE(model.move(INT_MAX));
    REQUIRE_FALSE(model.activate());
    REQUIRE(model.page() == 0);
    REQUIRE(model.move(INT_MIN));
    REQUIRE(model.focus() == 0);
    REQUIRE_FALSE(model.move(INT_MIN));
    REQUIRE_FALSE(model.move(0));
    REQUIRE_FALSE(model.result().accepted);
}
