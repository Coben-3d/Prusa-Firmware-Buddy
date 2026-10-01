#pragma once

#include <filament.hpp>
#include <string_view_utf8.hpp>
#include <screen_menu.hpp>
#include <screen_fsm.hpp>
#include <filament_list.hpp>
#include <dynamic_index_mapping.hpp>
#include <window_menu_virtual.hpp>
#include <window_menu_callback_item.hpp>
#include <printers.h>
#if PRINTER_IS_PRUSA_MK4()
    #include <menu_item/menu_item_select_menu.hpp>
#endif

#include <MItem_tools.hpp>
#include <fsm_preheat_type.hpp>

namespace preheat_menu {

class WindowMenuPreheat;

// extra space at the end is intended
class MI_FILAMENT : public WiInfo<sizeof("999/999 ")> {
public:
    MI_FILAMENT(FilamentType filament_type, uint8_t target_extruder);
    void click(IWindowMenu &) final;

    const FilamentType filament_type;
    const uint8_t target_extruder;
    FilamentTypeParameters::Name filament_name;
};

#if PRINTER_IS_PRUSA_MK4()
class MI_FILAMENT_COLOR final : public MenuItemSelectMenu {
public:
    MI_FILAMENT_COLOR();
    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) final;

private:
    std::optional<Color> custom_color;
};
#endif

class WindowMenuPreheat : public WindowMenuVirtual<WindowMenuCallbackItem, MI_FILAMENT
#if PRINTER_IS_PRUSA_MK4()
    , MI_FILAMENT_COLOR
#endif
    > {

public:
    WindowMenuPreheat(window_t *parent, const Rect16 &rect);

    void set_data(const PreheatData &data);
    void set_show_all_filaments(bool set);

    int item_count() const final {
        return index_mapping.total_item_count();
    }

    static bool handle_filament_selection(FilamentType filament_type, uint8_t target_extruder);

protected:
    void update_list();
    void setup_item(ItemVariant &variant, int index) final;

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    enum class Item {
        return_,
#if PRINTER_IS_PRUSA_MK4()
        color,
#endif
        filament_section,
        show_all,
        cooldown,
        adhoc_filament,
    };

    static constexpr auto items = std::to_array<DynamicIndexMappingRecord<Item>>({
        { Item::return_, DynamicIndexMappingType::optional_item },
#if PRINTER_IS_PRUSA_MK4()
        { Item::color, DynamicIndexMappingType::optional_item },
#endif
        { Item::filament_section, DynamicIndexMappingType::dynamic_section },
        { Item::adhoc_filament },
        { Item::show_all, DynamicIndexMappingType::optional_item },
        { Item::cooldown, DynamicIndexMappingType::optional_item },
    });

private:
    FilamentList filament_list;
    DynamicIndexMapping<items> index_mapping;
    bool show_all_filaments_ = false;

    /// Extruder we're doing the load/preheat for
    uint8_t extruder_index = 0;
};

class ScreenPreheat : public ScreenFSM {
    WindowExtendedMenu<WindowMenuPreheat> menu;

public:
    ScreenPreheat();

protected:
    void create_frame();
    void destroy_frame();
    void update_frame();
};

}; // namespace preheat_menu

using ScreenPreheat = preheat_menu::ScreenPreheat;
