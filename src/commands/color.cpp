#include "palette/commands/color.hpp"
#include "palette/services/color_api.hpp"
#include "palette/services/color_utils.hpp"
#include <nlohmann/json.hpp>

namespace palette::commands {
namespace {
void handle_color_impl(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    const services::single_color_input_result input =
        services::parse_single_color_input(event);
    if (!input.ok) {
        event.reply(input.error);
        return;
    }

    const std::string &query_key = input.query_key;
    const std::string &query_value = input.query_value;

    event.thinking();
    const std::string token = event.command.token;

    services::fetch_color(
        bot, query_key, query_value,
        [&bot, token, query_key, query_value](bool ok, std::string body) {
            if (!ok) {
                bot.interaction_followup_create(
                    token, dpp::message("API error: " + body));
                return;
            }

            try {
                auto j = nlohmann::json::parse(body);
                std::string name = j.at("name").at("value").get<std::string>();
                int r = j.at("rgb").at("r").get<int>();
                int g = j.at("rgb").at("g").get<int>();
                int b = j.at("rgb").at("b").get<int>();
                std::string thumbnail =
                    j.at("image").at("bare").get<std::string>();
                std::string hsl_value =
                    j.at("hsl").at("value").get<std::string>();
                std::string hsv_value =
                    j.at("hsv").at("value").get<std::string>();
                std::string rgb_value =
                    j.at("rgb").at("value").get<std::string>();
                std::string cmyk_value =
                    j.at("cmyk").at("value").get<std::string>();
                std::string xyz_value =
                    j.at("XYZ").at("value").get<std::string>();
                std::string hex_value =
                    j.at("hex").at("value").get<std::string>();
                bool match_name =
                    j.at("name").at("exact_match_name").get<bool>();
                double distance = j.at("name").at("distance").get<double>();

                std::string api_url =
                    services::build_id_url(query_key, query_value);
                if (j.contains("_links") && j["_links"].contains("self") &&
                    j["_links"]["self"].is_object() &&
                    j["_links"]["self"].contains("href") &&
                    j["_links"]["self"]["href"].is_string()) {
                    std::string href =
                        j["_links"]["self"]["href"].get<std::string>();
                    if (!href.empty() && href.front() == '/') {
                        api_url = "https://www.thecolorapi.com" + href;
                    } else if (!href.empty()) {
                        api_url = href;
                    }
                }

                uint32_t color = (r << 16) | (g << 8) | b;
                const bool is_web_safe =
                    services::is_web_safe_color({static_cast<uint8_t>(r),
                                                 static_cast<uint8_t>(g),
                                                 static_cast<uint8_t>(b)});
                std::string description =
                    "- **HEX:** " + hex_value +
                    "\n"
                    "- **RGB:** " +
                    rgb_value +
                    "\n"
                    "- **HSL:** " +
                    hsl_value +
                    "\n"
                    "- **HSV:** " +
                    hsv_value +
                    "\n"
                    "- **CMYK:** " +
                    cmyk_value +
                    "\n"
                    "- **XYZ:** " +
                    xyz_value +
                    "\n\n"
                    "**Details**\n"
                    "- **Exact name match:** " +
                    std::string(match_name ? "Yes" : "No") +
                    "\n"
                    "- **Name distance:** " +
                    std::to_string(static_cast<int>(distance)) + " (" +
                    services::name_distance_label(distance) +
                    ")\n\n**Web Safe Color**\n"
                    "- **Is web safe:** " +
                    std::string(is_web_safe ? "Yes" : "No") +
                    "\n"
                    "A web safe color will appear consistently across "
                    "devices, especially legacy 256-color environments.";

                std::string image_url = "https://images.weserv.nl/?url=" +
                                        dpp::utility::url_encode(thumbnail) +
                                        "&output=png&w=800&h=600";

                dpp::embed embed =
                    dpp::embed()
                        .set_color(color)
                        .set_url(
                            services::resolve_self_url(
                                j, services::build_id_url("hex", query_key)) +
                            "&format=html")
                        .set_title(name)
                        .set_url(api_url)
                        .set_description(description)
                        .set_image(image_url);

                bot.interaction_followup_create(token, embed);
            } catch (const std::exception &e) {
                bot.interaction_followup_create(
                    token, dpp::message("Failed to parse color response: " +
                                        std::string(e.what())));
            }
        });
}
} // namespace

void handle_color(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    handle_color_impl(bot, event);
}
} // namespace palette::commands
