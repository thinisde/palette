#include "palette/commands/contrast.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace palette::commands {
namespace {
std::string quality_label(int pass_count) {
    if (pass_count >= 4) {
        return "perfect";
    }
    if (pass_count >= 3) {
        return "good";
    }
    if (pass_count >= 2) {
        return "fair";
    }
    return "poor";
}

std::string pass_fail(bool pass) { return pass ? "Pass" : "Fail"; }

std::vector<std::string> contrast_image_lines(const std::string &hex_no_hash,
                                              const std::string &background_name,
                                              int pass_count,
                                              int rating_percent) {
    return {
        "COLOR " + hex_no_hash,
        "CONTRAST ON " + background_name,
        "PASS " + std::to_string(pass_count) + " OF 4",
        std::to_string(rating_percent) + " PERCENT RATING",
    };
}

void handle_contrast_test(const dpp::slashcommand_t &event,
                          services::rgb_color background,
                          const std::string &background_name,
                          bool background_is_black) {
    const services::single_color_input_result input =
        services::parse_single_color_input(event);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }

    services::rgb_color text_color{};
    if (!services::parse_query_color_to_rgb(input.query_key, input.query_value,
                                            text_color)) {
        event.reply("Could not parse the input value for `" + input.query_key +
                    "`.");
        return;
    }

    const services::wcag_contrast_result result =
        services::evaluate_wcag_contrast(text_color, background);
    const std::string hex = services::rgb_to_hex(text_color);
    const std::string hex_no_hash = hex.empty() ? hex : hex.substr(1);

    std::ostringstream ratio_ss;
    ratio_ss << std::fixed << std::setprecision(2) << result.ratio;

    std::string description =
        hex + " Contrast on " + background_name + " Background\n\n" +
        "The color " + hex + " has passed " + std::to_string(result.pass_count) +
        " out of 4 WCAG tests against a " + background_name +
        " background. This means there is a " +
        std::to_string(result.rating_percent) +
        "% accessibility rating and therefore makes " + hex + " a " +
        quality_label(result.pass_count) + " choice against a " +
        background_name + " background.\n\n" + "- Contrast ratio: " +
        ratio_ss.str() + ":1\n" + "- AA normal text (4.5:1): " +
        pass_fail(result.aa_normal) + "\n" + "- AA large text (3.0:1): " +
        pass_fail(result.aa_large) + "\n" + "- AAA normal text (7.0:1): " +
        pass_fail(result.aaa_normal) + "\n" + "- AAA large text (4.5:1): " +
        pass_fail(result.aaa_large);

    dpp::message msg(event.command.channel_id, description);
    const std::vector<std::string> lines =
        contrast_image_lines(hex_no_hash, background_name, result.pass_count,
                             result.rating_percent);
    const std::string image_data =
        background_is_black
            ? services::generate_text_on_black_image(lines, text_color)
            : services::generate_text_on_white_image(lines, text_color);
    if (!image_data.empty()) {
        msg.add_file(background_is_black ? "black-contrast.png"
                                         : "white-contrast.png",
                     image_data);
    }

    event.reply(msg);
}

std::string to_lower_ascii(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
} // namespace

void handle_contrast(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    (void)bot;

    auto background_param = event.get_parameter("background");
    std::string background = "black";
    if (const auto *p = std::get_if<std::string>(&background_param)) {
        background = to_lower_ascii(*p);
    }

    if (background == "black") {
        handle_contrast_test(event, {0, 0, 0}, "Black", true);
        return;
    }
    if (background == "white") {
        handle_contrast_test(event, {255, 255, 255}, "White", false);
        return;
    }

    event.reply("`background` must be `black` or `white`.");
}

} // namespace palette::commands
