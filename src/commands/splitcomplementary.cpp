#include "palette/commands/splitcomplementary.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <cmath>
#include <string>
#include <vector>

namespace palette::commands {
namespace {
double normalize_hue(double h) {
    h = std::fmod(h, 360.0);
    if (h < 0.0) {
        h += 360.0;
    }
    return h;
}

std::string hsl_label(double h, double s, double l) {
    const int hi = static_cast<int>(std::round(h));
    const int si = static_cast<int>(std::round(s));
    const int li = static_cast<int>(std::round(l));
    return "hsl(" + std::to_string(hi) + "," + std::to_string(si) + "%," +
           std::to_string(li) + "%)";
}
} // namespace

void handle_splitcomplementary(dpp::cluster &bot,
                               const dpp::slashcommand_t &event) {
    (void)bot;

    const services::single_color_input_result input =
        services::parse_single_color_input(event);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }

    services::rgb_color base{};
    if (!services::parse_query_color_to_rgb(input.query_key, input.query_value,
                                            base)) {
        event.reply("Could not parse the input value for `" + input.query_key +
                    "`.");
        return;
    }

    double h = 0.0;
    double s = 0.0;
    double l = 0.0;
    services::rgb_to_hsl(base, h, s, l);

    const double left_h = normalize_hue(h + 150.0);
    const double right_h = normalize_hue(h + 210.0);

    services::rgb_color left{};
    services::rgb_color right{};
    services::hsl_to_rgb(left_h, s, l, left);
    services::hsl_to_rgb(right_h, s, l, right);

    std::string description =
        "A split complementary scheme uses a base color and two colors around "
        "its complementary hue.\n\n"
        "1. **Original:** " +
        services::rgb_to_hex(base) + " | " + hsl_label(h, s, l) +
        "\n"
        "2. **Left of Complement:** " +
        services::rgb_to_hex(left) + " | " + hsl_label(left_h, s, l) +
        "\n"
        "3. **Right of Complement:** " +
        services::rgb_to_hex(right) + " | " + hsl_label(right_h, s, l);

    dpp::message msg(event.command.channel_id, description);
    const std::string image_data =
        services::generate_palette_image({base, left, right}, true);
    if (!image_data.empty()) {
        msg.add_file("split-complementary-palette.png", image_data);
    }
    event.reply(msg);
}

} // namespace palette::commands
