#include "palette/commands/websafe.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <string>
#include <vector>

namespace palette::commands {
void handle_websafe(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    (void)bot;

    const services::single_color_input_result input =
        services::parse_single_color_input(event);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }

    services::rgb_color original{};
    if (!services::parse_query_color_to_rgb(input.query_key, input.query_value,
                                            original)) {
        event.reply("Could not parse the input value for `" + input.query_key +
                    "`.");
        return;
    }

    const services::rgb_color websafe = services::nearest_web_safe_color(original);
    const bool already_websafe = services::is_web_safe_color(original);

    std::string description =
        "**Web Safe Color**\n"
        "A web safe color is a color that will not be dithered in 256-color "
        "environments. There are 216 web safe colors.\n\n"
        "1. **Original:** " +
        services::rgb_to_hex(original) +
        "\n"
        "2. **Nearest web safe:** " +
        services::rgb_to_hex(websafe) +
        "\n"
        "- **Already web safe:** " +
        std::string(already_websafe ? "Yes" : "No");

    dpp::message msg(event.command.channel_id, description);
    const std::string image_data =
        services::generate_palette_image({original, websafe}, true);
    if (!image_data.empty()) {
        msg.add_file("websafe-palette.png", image_data);
    }
    event.reply(msg);
}

} // namespace palette::commands
