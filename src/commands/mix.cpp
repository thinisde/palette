#include "palette/commands/mix.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <string>
#include <vector>

namespace palette::commands {
namespace {
std::string rgb_label(services::rgb_color color) {
    return "rgb(" + std::to_string(static_cast<int>(color.r)) + "," +
           std::to_string(static_cast<int>(color.g)) + "," +
           std::to_string(static_cast<int>(color.b)) + ")";
}
} // namespace

void handle_mix(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    (void)bot;

    const services::multi_color_input_result input =
        services::parse_multi_color_input(event, 3);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }
    if (input.colors.size() < 2) {
        event.reply("Provide at least 2 colors to mix.");
        return;
    }

    const services::rgb_color mixed = services::mix_colors(input.colors);
    std::vector<services::rgb_color> palette_colors = input.colors;
    palette_colors.push_back(mixed);

    std::string description =
        "**Color Mixing**\n"
        "Mixing blends source colors by averaging their RGB channels.\n\n"
        "**Inputs**\n";
    for (size_t i = 0; i < input.colors.size(); ++i) {
        const services::rgb_color c = input.colors[i];
        description += std::to_string(i + 1) + ". " + services::rgb_to_hex(c) +
                       " | " + rgb_label(c) + "\n";
    }

    description += "\n**Mixed Result**\n" +
                   std::to_string(input.colors.size() + 1) + ". " +
                   services::rgb_to_hex(mixed) + " | " + rgb_label(mixed);

    dpp::message msg(event.command.channel_id, description);
    const std::string image_data =
        services::generate_palette_image(palette_colors, true);
    if (!image_data.empty()) {
        msg.add_file("mixed-palette.png", image_data);
    }

    event.reply(msg);
}

} // namespace palette::commands
