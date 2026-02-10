#include "palette/buttons/shades.hpp"
#include "palette/services/palette_controls.hpp"
#include <string>

namespace palette::buttons {

void handle_shades(dpp::cluster &bot, const dpp::button_click_t &event) {
    (void)bot;

    services::palette_control_mode mode = services::palette_control_mode::shades;
    int delta = 0;
    std::string token;
    if (!services::parse_palette_button_id(event.custom_id, mode, delta, token) ||
        mode != services::palette_control_mode::shades) {
        event.reply("Unknown shades action.");
        return;
    }

    services::palette_control_state state;
    if (!services::adjust_palette_control_amount(token, delta, state)) {
        event.reply("This shades session expired. Run `/shades` again.");
        return;
    }

    const services::palette_render_result rendered =
        services::render_palette_with_controls(state.mode, state.seed_colors,
                                               state.amount);
    if (!rendered.ok) {
        event.reply(rendered.error);
        return;
    }

    dpp::message msg;
    msg.set_content(rendered.description);
    msg.add_component(services::build_palette_controls_row(
        state.mode, state.amount, token));
    msg.add_file("color-palette.png", rendered.image_data);
    event.reply(dpp::ir_update_message, msg);
}

} // namespace palette::buttons
