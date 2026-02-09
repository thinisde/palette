#include "palette/commands/scheme.hpp"
#include "palette/services/color_api.hpp"
#include "palette/services/palette_image.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

namespace palette::commands {
namespace {
constexpr std::array<const char *, 8> kModes = {
    "monochrome",
    "monochrome-dark",
    "monochrome-light",
    "analogic",
    "complement",
    "analogic-complement",
    "triad",
    "quad",
};

std::string to_lower_ascii(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool is_valid_mode(const std::string &mode) {
    return std::find(kModes.begin(), kModes.end(), mode) != kModes.end();
}

std::string clean_hex_code(std::string hex) {
    if (!hex.empty() && hex[0] == '#') {
        hex.erase(0, 1);
    }
    return hex;
}

std::string build_scheme_url(const std::string &hex, const std::string &mode,
                             int count) {
    return "https://www.thecolorapi.com/scheme?hex=" + clean_hex_code(hex) +
           "&mode=" + dpp::utility::url_encode(mode) +
           "&count=" + std::to_string(count);
}

int parse_count(const nlohmann::json &json, int fallback) {
    if (!json.contains("count")) {
        return fallback;
    }

    const auto &value = json["count"];
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    if (value.is_string()) {
        try {
            return std::stoi(value.get<std::string>());
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}
} // namespace

void handle_scheme(dpp::cluster &bot, const dpp::slashcommand_t &event) {
    auto hex_param = event.get_parameter("hex");
    auto mode_param = event.get_parameter("mode");
    auto count_param = event.get_parameter("count");

    std::string hex;
    if (auto p = std::get_if<std::string>(&hex_param)) {
        hex = *p;
    } else {
        event.reply("`hex` must be a string.");
        return;
    }

    std::string mode = "monochrome";
    if (auto p = std::get_if<std::string>(&mode_param)) {
        mode = to_lower_ascii(*p);
    }

    if (!is_valid_mode(mode)) {
        event.reply("`mode` must be one of: monochrome, monochrome-dark, "
                    "monochrome-light, analogic, complement, "
                    "analogic-complement, triad, quad.");
        return;
    }

    int count = 5;
    if (auto p = std::get_if<int64_t>(&count_param)) {
        count = static_cast<int>(*p);
    }
    count = std::clamp(count, 1, 20);

    event.thinking();
    const std::string token = event.command.token;

    services::fetch_scheme(
        bot, hex, mode, count,
        [&bot, token, mode, count](bool ok, std::string body) {
            if (!ok) {
                bot.interaction_followup_create(
                    token, dpp::message("API error: " + body));
                return;
            }

            try {
                auto json = nlohmann::json::parse(body);
                const auto &seed = json.at("seed");
                const auto &colors = json.at("colors");

                std::string scheme_mode = mode;
                if (json.contains("mode") && json["mode"].is_string()) {
                    scheme_mode = json["mode"].get<std::string>();
                }
                int scheme_count = parse_count(json, count);

                std::string seed_hex =
                    seed.at("hex").at("clean").get<std::string>();
                std::string seed_name =
                    seed.at("name").at("value").get<std::string>();
                std::string seed_rgb =
                    seed.at("rgb").at("value").get<std::string>();

                std::string url =
                    build_scheme_url(seed_hex, scheme_mode, scheme_count);
                if (json.contains("_links") &&
                    json["_links"].contains("self") &&
                    json["_links"]["self"].is_string()) {
                    const std::string self_path =
                        json["_links"]["self"].get<std::string>();
                    if (!self_path.empty() && self_path.front() == '/') {
                        url = "https://www.thecolorapi.com" + self_path;
                    }
                }

                std::vector<services::rgb_color> palette_colors;
                palette_colors.reserve(colors.size());
                std::string description =
                    "A monochromatic color scheme is based on a single color "
                    "and contains both shades and tints of that color.\n\n"
                    "**Scheme**\n"
                    "- **Seed:** #" +
                    seed_hex + " (" + seed_name +
                    ")\n"
                    "- **RGB:** " +
                    seed_rgb +
                    "\n"
                    "- **Mode:** " +
                    scheme_mode +
                    "\n"
                    "- **Count:** " +
                    std::to_string(colors.size()) +
                    "\n"
                    "- **URL:** [Open](" +
                    url + "&format=html" + ")\n\n";

                size_t idx = 1;
                for (const auto &color : colors) {
                    const std::string hex_value =
                        color.at("hex").at("value").get<std::string>();
                    const std::string name_value =
                        color.at("name").at("value").get<std::string>();
                    const std::string rgb_value =
                        color.at("rgb").at("value").get<std::string>();
                    const std::string hsl_value =
                        color.at("hsl").at("value").get<std::string>();
                    const int r = color.at("rgb").at("r").get<int>();
                    const int g = color.at("rgb").at("g").get<int>();
                    const int b = color.at("rgb").at("b").get<int>();

                    palette_colors.push_back({static_cast<uint8_t>(r),
                                              static_cast<uint8_t>(g),
                                              static_cast<uint8_t>(b)});

                    description += std::to_string(idx) + ". " + hex_value +
                                   " " + name_value + " | " + rgb_value +
                                   " | " + hsl_value + "\n";
                    ++idx;
                }

                dpp::message msg(description);
                const std::string image_data =
                    services::generate_palette_image(palette_colors, true);
                if (!image_data.empty()) {
                    msg.add_file("scheme-palette.png", image_data);
                }

                bot.interaction_followup_create(token, msg);
            } catch (const std::exception &e) {
                bot.interaction_followup_create(
                    token, dpp::message("Failed to parse scheme: " +
                                        std::string(e.what())));
            }
        });
}

} // namespace palette::commands
