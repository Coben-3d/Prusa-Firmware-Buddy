#include "dialog_filament_color.hpp"

#include <IDialog.hpp>
#include <ScreenHandler.hpp>
#include <window_header.hpp>
#include <display.hpp>
#include <display_helper.h>
#include <gui/event/knob_event.hpp>
#include <i18n.h>
#include <sound.hpp>
#include <cstdio>

namespace {

using Model = filament::ColorPaletteModel;
static_assert(GuiDefaults::ScreenWidth == 480 && GuiDefaults::ScreenHeight == 320);

constexpr int grid_x = 12;
constexpr int grid_y = GuiDefaults::HeaderHeight + 8;
constexpr int cell_width = 70;
constexpr int cell_height = 44;
constexpr int pitch_x = 76;
constexpr int pitch_y = 50;
constexpr int controls_y = 278;

constexpr Rect16 cell_rect(int index) {
    return Rect16(grid_x + (index % Model::columns) * pitch_x,
        grid_y + (index / Model::columns) * pitch_y, cell_width, cell_height);
}
constexpr Rect16 control_rect(int index) {
    switch (index) {
    case Model::back: return Rect16(12, controls_y, 96, 34);
    case Model::unknown: return Rect16(114, controls_y, 134, 34);
    case Model::previous_page: return Rect16(254, controls_y, 52, 34);
    default: return Rect16(312, controls_y, 52, 34);
    }
}
static_assert(grid_y + 3 * pitch_y + cell_height <= 244);
static_assert(controls_y + 34 <= GuiDefaults::ScreenHeight);

class ColorPaletteWindow final : public window_t {
public:
    ColorPaletteWindow(window_t *parent, std::optional<Color> initial)
        : window_t(parent, GuiDefaults::RectScreenNoHeader), model(initial) {
        Enable();
    }

    Model model;

protected:
    void unconditionalDraw() final {
        display::fill_rect(GetRect(), COLOR_BLACK);
        for (int i = 0; i < model.color_count(); ++i) {
            const auto rect = cell_rect(i);
            // A neutral border makes black and pale swatches visible. The orange
            // double border is the encoder focus, not a stored color change.
            display::fill_rect(rect, model.focus() == i ? COLOR_BRAND : COLOR_SILVER);
            display::fill_rect(Rect16(rect.Left() + 3, rect.Top() + 3, rect.Width() - 6, rect.Height() - 6), model.color_at(i));
            if (model.focus() == i) {
                const auto outline = model.color_at(i).to_grayscale() > 127 ? COLOR_BLACK : COLOR_WHITE;
                display::draw_rect(Rect16(rect.Left() + 3, rect.Top() + 3, rect.Width() - 6, rect.Height() - 6), outline);
            }
            if (model.initial() == model.color_at(i)) {
                // Small contrasting marker for the existing pending choice.
                const auto mark = model.color_at(i).to_grayscale() > 127 ? COLOR_BLACK : COLOR_WHITE;
                display::fill_rect(Rect16(rect.Left() + 8, rect.Top() + 8, 6, 6), mark);
            }
        }

        const auto preview = model.preview();
        char hex[8] {};
        const auto value = preview
            ? (snprintf(hex, sizeof(hex), "#%06lX", static_cast<unsigned long>(preview->raw)), string_view_utf8::MakeRAM(hex))
            : _("Unknown");
        display::fill_rect(Rect16(12, 248, 28, 24), COLOR_SILVER);
        if (preview) {
            display::fill_rect(Rect16(14, 250, 24, 20), *preview);
        } else {
            display::draw_line({ 15, 251 }, { 36, 268 }, COLOR_BLACK);
            display::draw_line({ 15, 268 }, { 36, 251 }, COLOR_BLACK);
        }
        render_text_align(Rect16(50, 246, 220, 28), value, Font::normal, COLOR_BLACK, COLOR_WHITE, {}, Align_t::LeftCenter());

        for (int index = Model::back; index <= Model::next_page; ++index) {
            const auto focused = model.focus() == index;
            const auto bg = focused ? COLOR_WHITE : COLOR_VERY_DARK_GRAY;
            display::fill_rect(control_rect(index), bg);
            string_view_utf8 label;
            switch (index) {
            case Model::back: label = _("Back"); break;
            case Model::unknown: label = _("Unknown"); break;
            case Model::previous_page: label = string_view_utf8::MakeCPUFLASH("<"); break;
            default: label = string_view_utf8::MakeCPUFLASH(">"); break;
            }
            render_text_align(control_rect(index), label, Font::normal, bg, focused ? COLOR_BLACK : COLOR_WHITE, {}, Align_t::Center());
        }
        char page[8];
        snprintf(page, sizeof(page), "%d/%d", model.page() + 1, Model::page_count);
        render_text_align(Rect16(374, controls_y, 94, 34), string_view_utf8::MakeRAM(page), Font::normal, COLOR_BLACK, COLOR_WHITE, {}, Align_t::Center());
    }

    void windowEvent(window_t *sender, GUI_event_t event, void *param) final {
        switch (event) {
        case GUI_event_t::KNOB: {
            auto &ctx = *static_cast<GuiEventContext *>(param);
            if (model.move(ctx.event.value<gui_event::KnobEvent>().diff)) {
                sound::play(SoundType::encoder_move);
                Invalidate();
            }
            ctx.accept();
            return;
        }
        case GUI_event_t::CLICK:
            activate();
            return;
        case GUI_event_t::TOUCH_CLICK: {
            const auto point = event_conversion_union { .pvoid = param }.point;
            for (int i = 0; i < model.color_count(); ++i) {
                if (cell_rect(i).Contain(point)) {
                    model.set_focus(i);
                    activate();
                    return;
                }
            }
            for (int i = Model::back; i <= Model::next_page; ++i) {
                if (control_rect(i).Contain(point)) {
                    model.set_focus(i);
                    activate();
                    return;
                }
            }
            return; // Gaps and unused cells never commit a color.
        }
        default:
            window_t::windowEvent(sender, event, param);
            break;
        }
    }

    void screenEvent(window_t *sender, GUI_event_t event, void *param) final {
        if (event == GUI_event_t::TOUCH_SWIPE_LEFT || event == GUI_event_t::TOUCH_SWIPE_RIGHT) {
            // Keep the same back gesture as the existing selection submenu.
            sound::play(SoundType::button_echo);
            Screens::Access()->Close();
            return;
        }
        window_t::screenEvent(sender, event, param);
    }

private:
    void activate() {
        sound::play(SoundType::button_echo);
        if (model.activate()) Screens::Access()->Close();
        else Invalidate();
    }
};

class ColorPaletteDialog final : public IDialog {
public:
    explicit ColorPaletteDialog(std::optional<Color> initial)
        : IDialog(GuiDefaults::RectScreen), header(this, _("Filament Color")), palette(this, initial) {
        CaptureNormalWindow(palette);
        palette.SetFocus();
    }

    window_header_t header;
    ColorPaletteWindow palette;
};
static_assert(sizeof(ColorPaletteDialog) <= 768); // Bounded modal stack footprint.
} // namespace

filament::ColorPaletteResult select_filament_color_dialog(std::optional<Color> initial) {
    ColorPaletteDialog dialog(initial);
    Screens::Access()->gui_loop_until_dialog_closed();
    return dialog.palette.model.result();
}
