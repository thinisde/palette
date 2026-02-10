#include "palette/commands/tints.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_controls.hpp"

namespace palette::commands {

void handle_tints(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    (void)bot;

    auto amount_param = event.get_parameter("amount");
    int amount = 0;
    if (const auto *p = std::get_if<int64_t>(&amount_param)) {
        amount = static_cast<int>(*p);
    } else {
        event.reply("`amount` is required and must be an integer from 2 to 8.");
        return;
    }
    if (amount < 2 || amount > 8) {
        event.reply("`amount` must be between 2 and 8.");
        return;
    }

    const services::multi_color_input_result input =
        services::parse_multi_color_input(event, 10);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }

    const std::string token = services::create_palette_control_token(
        services::palette_control_mode::tints, input.colors, amount);
    if (token.empty()) {
        event.reply("Failed to initialize tints session.");
        return;
    }

    const services::palette_render_result rendered =
        services::render_palette_with_controls(services::palette_control_mode::tints,
                                               input.colors, amount);
    if (!rendered.ok) {
        event.reply(rendered.error);
        return;
    }

    dpp::message msg(event.command.channel_id, rendered.description);
    msg.add_component(services::build_palette_controls_row(
        services::palette_control_mode::tints, amount, token));
    msg.add_file("tint-palette.png", rendered.image_data);
    event.reply(msg);
}

} // namespace palette::commands
