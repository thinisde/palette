#include "palette/commands/shades.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <string>
#include <vector>

namespace palette::commands {
namespace {
std::string format_palette_details(
    const std::vector<std::vector<services::rgb_color>> &palette) {
    std::string out;
    for (size_t row = 0; row < palette.size(); ++row) {
        const auto &shades = palette[row];
        if (shades.empty()) {
            continue;
        }

        if (palette.size() > 1) {
            out += "**Color " + std::to_string(row + 1) + "**\n";
        }

        for (size_t i = 0; i < shades.size(); ++i) {
            out += std::to_string(i + 1) + ". " + services::rgb_to_hex(shades[i]);
            if (i == 0) {
                out += " (original)";
            } else if (i + 1 == shades.size()) {
                out += " (black)";
            }
            out += "\n";
        }

        if (row + 1 != palette.size()) {
            out += "\n";
        }
    }
    return out;
}
} // namespace

void handle_shades(dpp::cluster &bot, const dpp::slashcommand_t &event) {
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

    const services::image_result image =
        services::generate_color_palette(input.colors, amount);
    if (image.image_data.empty()) {
        event.reply("Failed to generate color palette image.");
        return;
    }

    std::string description = format_palette_details(image.palette);
    if (description.empty()) {
        description = "Generated color palette.";
    } else {
        description =
            "A shade is a concept of darkening a color.\n\n" + description;
    }

    dpp::message msg(event.command.channel_id, description);
    msg.add_file("color-palette.png", image.image_data);
    event.reply(msg);
}

} // namespace palette::commands
