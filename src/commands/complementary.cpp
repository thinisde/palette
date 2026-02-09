#include "palette/commands/complementary.hpp"
#include "palette/services/color_api.hpp"
#include "palette/services/color_utils.hpp"
#include "palette/services/palette_image.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace palette::commands {
void handle_complementary(dpp::cluster &bot, const dpp::slashcommand_t &event) {
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
                int h = j.at("hsl").at("h").get<int>();
                int s = j.at("hsl").at("s").get<int>();
                int l = j.at("hsl").at("l").get<int>();
                std::string seed_hex =
                    j.at("hex").at("value").get<std::string>();
                std::string seed_rgb =
                    j.at("rgb").at("value").get<std::string>();
                std::string seed_hsl =
                    j.at("hsl").at("value").get<std::string>();
                int seed_r = j.at("rgb").at("r").get<int>();
                int seed_g = j.at("rgb").at("g").get<int>();
                int seed_b = j.at("rgb").at("b").get<int>();

                int comp_h = (h + 180) % 360;
                int comp_s = s;
                int comp_l = l;

                services::rgb_color comp_rgb{};
                services::hsl_to_rgb(comp_h, comp_s, comp_l, comp_rgb);

                std::string comp_hex = services::rgb_to_hex(comp_rgb);
                std::string original_url = services::resolve_self_url(
                    j, services::build_id_url(query_key, query_value));

                services::fetch_color(
                    bot, "hex", comp_hex,
                    [&bot, token, name, seed_hex, seed_rgb, seed_hsl, comp_hex,
                     original_url, seed_r, seed_g,
                     seed_b](bool comp_ok, std::string comp_body) {
                        if (!comp_ok) {
                            bot.interaction_followup_create(
                                token,
                                dpp::message("API error (complement lookup): " +
                                             comp_body));
                            return;
                        }

                        try {
                            auto comp_json = nlohmann::json::parse(comp_body);
                            std::string comp_name = comp_json.at("name")
                                                        .at("value")
                                                        .get<std::string>();
                            std::string comp_hex_api = comp_json.at("hex")
                                                           .at("value")
                                                           .get<std::string>();
                            std::string comp_rgb_api = comp_json.at("rgb")
                                                           .at("value")
                                                           .get<std::string>();
                            std::string comp_hsl_api = comp_json.at("hsl")
                                                           .at("value")
                                                           .get<std::string>();
                            int comp_r_api =
                                comp_json.at("rgb").at("r").get<int>();
                            int comp_g_api =
                                comp_json.at("rgb").at("g").get<int>();
                            int comp_b_api =
                                comp_json.at("rgb").at("b").get<int>();
                            std::string description =
                                "Complementary colors are opposite on the "
                                "color wheel. Using them together creates "
                                "strong visual contrast.\n\n"
                                "**Your provided color**\n"
                                "- **Name:** " +
                                name +
                                "\n"
                                "- **HEX:** " +
                                seed_hex +
                                "\n"
                                "- **RGB:** " +
                                seed_rgb +
                                "\n"
                                "- **HSL:** " +
                                seed_hsl +
                                "\n"
                                "- **URL:** [Open](" +
                                original_url +
                                ")\n\n"
                                "**Complement**\n"
                                "- **Name:** " +
                                comp_name +
                                "\n"
                                "- **HEX:** " +
                                comp_hex_api +
                                "\n"
                                "- **RGB:** " +
                                comp_rgb_api +
                                "\n"
                                "- **HSL:** " +
                                comp_hsl_api + "\n - **URL:** [Open](" +
                                services::build_id_url("hex", comp_hex_api) +
                                "&format=html" + ")\n\n";
                            dpp::message msg(description);

                            std::vector<services::rgb_color> palette_colors = {
                                {static_cast<uint8_t>(seed_r),
                                 static_cast<uint8_t>(seed_g),
                                 static_cast<uint8_t>(seed_b)},
                                {static_cast<uint8_t>(comp_r_api),
                                 static_cast<uint8_t>(comp_g_api),
                                 static_cast<uint8_t>(comp_b_api)},
                            };

                            const std::string palette_image =
                                services::generate_palette_image(palette_colors,
                                                                 true);
                            if (!palette_image.empty()) {
                                msg.add_file("complementary-palette.png",
                                             palette_image);
                            }

                            bot.interaction_followup_create(token, msg);
                        } catch (const std::exception &e) {
                            bot.interaction_followup_create(
                                token,
                                dpp::message(
                                    "Failed to parse complement response: " +
                                    std::string(e.what())));
                        }
                    });
            } catch (const std::exception &e) {
                bot.interaction_followup_create(
                    token, dpp::message("Failed to parse color response: " +
                                        std::string(e.what())));
            }
        });
}

} // namespace palette::commands
