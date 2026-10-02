#include "screen_filament_colors.hpp"

#include <ScreenHandler.hpp>
#include <config_store/store_instance.hpp>
#include <dialog_filament_color.hpp>
#include <display.hpp>
#include <img_resources.hpp>
#include <loaded_filament_color_edit.hpp>
#include <marlin_client.hpp>
#include <marlin_vars.hpp>
#include <window_msgbox.hpp>
#include <option/has_coldpull.h>
#include <utils/string_builder.hpp>

namespace {
constexpr const char *busy_text = N_("Printer is busy. Please try repeating the action later.");

bool can_edit_colors() {
    if (!marlin_client::is_idle()) {
        return false;
    }
    return marlin_vars().peek_fsm_states([](const auto &states) {
        return !states.is_active(ClientFSM::Load_unload)
            && !states.is_active(ClientFSM::Preheat)
#if HAS_COLDPULL()
            && !states.is_active(ClientFSM::ColdPull)
#endif
            ;
    });
}
} // namespace

MI_FILAMENT_COLORS::MI_FILAMENT_COLORS()
    : IWindowMenuItem(_("Filament Colors"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {}

void MI_FILAMENT_COLORS::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenFilamentColors>();
}

MI_LOADED_FILAMENT_COLOR::MI_LOADED_FILAMENT_COLOR(uint8_t tool)
    : IWindowMenuItem({}, 144)
    , tool_(VirtualToolIndex::from_raw(tool)) {
    refresh();
}

void MI_LOADED_FILAMENT_COLOR::refresh() {
    auto &store = config_store();
    const auto material = store.get_filament_type(tool_);
    const auto declaration = store.loaded_filament_colors.get(tool_.to_raw());
    const auto color = filament::loaded_color_material(declaration) == EncodedFilamentType(material).data
        ? filament::decode_loaded_color(declaration) : std::nullopt;

    if (material != material_ || !label_buffer_[0]) {
        material_ = material;
        StringBuilder sb(label_buffer_);
        sb.append_string_view(_("Extruder"));
        sb.append_printf(" %u: ", tool_.display_index());
        material_.build_name_with_info(sb);
        SetLabel(string_view_utf8::MakeRAM(label_buffer_.data()));
        InValidateLabel();
    }
    if (color != color_) {
        color_ = color;
        InValidateExtension();
    }
    // Keep all eight head numbers visible, even when a head is empty/disabled.
    set_enabled(tool_.is_enabled() && material != FilamentType::none && can_edit_colors());
}

void MI_LOADED_FILAMENT_COLOR::Loop() {
    refresh();
}

void MI_LOADED_FILAMENT_COLOR::click(IWindowMenu &menu) {
    auto &store = config_store();
    const auto slot = tool_.to_raw();
    const auto operation_id = marlin_vars().peek_fsm_states([](const auto &states) { return states.get_state_id(); });
    const filament::LoadedColorEditSnapshot before {
        EncodedFilamentType(store.get_filament_type(tool_)).data,
        store.loaded_filament_colors.get(slot),
    };
    if (!tool_.is_enabled() || !before.material || !can_edit_colors()) {
        MsgBoxWarning(_(busy_text), Responses_Ok);
        refresh();
        return;
    }

    const auto initial = filament::loaded_color_material(before.declaration) == before.material
        ? filament::decode_loaded_color(before.declaration) : std::nullopt;
    const auto previous_focus = menu.focused_item_index();
    const auto choice = select_filament_color_dialog(initial);
    menu.move_focus_to_index(previous_focus);
    if (!choice.accepted) {
        refresh();
        return;
    }

    // Also reject an operation that started and finished while the palette
    // was open, even if it loaded the same material and identical RGB again.
    const bool operation_unchanged = marlin_vars().peek_fsm_states([&](const auto &states) {
        return states.get_state_id() == operation_id;
    });
    const bool available = tool_.is_enabled() && can_edit_colors() && operation_unchanged;
    const auto current_material = EncodedFilamentType(store.get_filament_type(tool_)).data;
    if (!filament::try_edit_loaded_color(store.loaded_filament_colors, slot, before,
            current_material, available, choice.color)) {
        MsgBoxWarning(_("Filament changed or printer is busy. Reopen the color palette."), Responses_Ok);
    }
    refresh();
}

void MI_LOADED_FILAMENT_COLOR::printExtension(Rect16 rect, Color text, Color background, ropfn) const {
    char hex[8] {};
    const auto value = color_
        ? (snprintf(hex, sizeof(hex), "#%06lX", static_cast<unsigned long>(color_->raw)), string_view_utf8::MakeRAM(hex))
        : _("Unknown");
    const int top = rect.Top() + (rect.Height() - 20) / 2;
    display::fill_rect(Rect16(rect.Left(), top, 24, 20), text);
    if (color_) {
        display::fill_rect(Rect16(rect.Left() + 2, top + 2, 20, 16), *color_);
    } else {
        display::draw_line({ static_cast<uint16_t>(rect.Left() + 3), static_cast<uint16_t>(top + 3) },
            { static_cast<uint16_t>(rect.Left() + 20), static_cast<uint16_t>(top + 16) }, background);
    }
    render_text_align(Rect16(rect.Left() + 32, rect.Top(), rect.Width() - 32, rect.Height()), value,
        GuiDefaults::FontMenuItems, background, text, {}, Align_t::RightCenter(), false);
}

ScreenFilamentColors::ScreenFilamentColors()
    : ScreenMenu(_("Filament Colors")) {
    static_assert(VirtualToolIndex::count == 8);
    header.SetIcon(&img::spool_16x16);
}
