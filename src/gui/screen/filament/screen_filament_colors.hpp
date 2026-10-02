#pragma once

#include <meta_utils.hpp>
#include <i_window_menu_item.hpp>
#include <WindowMenuItems.hpp>
#include <screen_menu.hpp>
#include <tool_index.hpp>

class MI_FILAMENT_COLORS final : public IWindowMenuItem {
public:
    MI_FILAMENT_COLORS();

protected:
    void click(IWindowMenu &) final;
};

class MI_LOADED_FILAMENT_COLOR : public IWindowMenuItem {
public:
    explicit MI_LOADED_FILAMENT_COLOR(uint8_t tool);

protected:
    void click(IWindowMenu &) final;
    void Loop() final;
    void printExtension(Rect16 rect, Color text, Color background, ropfn) const final;

private:
    void refresh();

    std::array<char, 48> label_buffer_ {};
    VirtualToolIndex tool_;
    FilamentType material_ = FilamentType::none;
    std::optional<Color> color_;
};

template <typename>
struct ScreenFilamentColors_ {};

template <size_t... i>
struct ScreenFilamentColors_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
        WithConstructorArgs<MI_LOADED_FILAMENT_COLOR, i>...>;
};

class ScreenFilamentColors final : public ScreenFilamentColors_<std::make_index_sequence<VirtualToolIndex::count>>::T {
public:
    ScreenFilamentColors();
};
