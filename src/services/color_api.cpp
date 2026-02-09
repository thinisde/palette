#include "palette/services/color_api.hpp"
#include <algorithm>

namespace palette::services {
namespace {
std::string clean_hex_code(std::string hex) {
    if (!hex.empty() && hex[0] == '#') {
        hex.erase(0, 1);
    }
    return hex;
}
} // namespace

void fetch_color(dpp::cluster &bot, const std::string &query_key,
                 const std::string &query_value,
                 std::function<void(bool, std::string)> cb) {
    std::string value = query_value;
    if (query_key == "hex") {
        value = clean_hex_code(value);
    }

    std::string url = "https://www.thecolorapi.com/id?" + query_key + "=" +
                      dpp::utility::url_encode(value) + "&format=json";

    bot.request(url, dpp::http_method::m_get,
                [cb](const dpp::http_request_completion_t &res) {
                    if (res.status != 200) {
                        cb(false, "HTTP " + std::to_string(res.status));
                        return;
                    }
                    cb(true, res.body);
                });
}

void fetch_scheme(dpp::cluster &bot, const std::string &hex,
                  const std::string &mode, int count,
                  std::function<void(bool, std::string)> cb) {
    std::string clean = clean_hex_code(hex);
    std::string resolved_mode = mode.empty() ? "monochrome" : mode;
    int resolved_count = std::clamp(count, 1, 20);

    std::string url = "https://www.thecolorapi.com/scheme?hex=" + clean +
                      "&mode=" + dpp::utility::url_encode(resolved_mode) +
                      "&count=" + std::to_string(resolved_count);

    bot.request(url, dpp::http_method::m_get,
                [cb](const dpp::http_request_completion_t &res) {
                    if (res.status != 200) {
                        cb(false, "HTTP " + std::to_string(res.status));
                        return;
                    }
                    cb(true, res.body);
                });
}

std::string name_distance_label(double d) {
    if (d == 0)
        return "Exact match";
    if (d <= 50)
        return "Very close";
    if (d <= 150)
        return "Close";
    if (d <= 300)
        return "Moderate";
    if (d <= 600)
        return "Far";
    return "Very far";
}

std::string resolve_self_url(const nlohmann::json &json,
                             const std::string &fallback_url) {
    if (json.contains("_links") && json["_links"].contains("self") &&
        json["_links"]["self"].is_object() &&
        json["_links"]["self"].contains("href") &&
        json["_links"]["self"]["href"].is_string()) {
        const std::string href =
            json["_links"]["self"]["href"].get<std::string>();
        if (!href.empty() && href.front() == '/') {
            return "https://www.thecolorapi.com" + href + "&format=html";
        }
        if (!href.empty()) {
            return href;
        }
    }
    return fallback_url;
}

std::string build_id_url(const std::string &query_key,
                         const std::string &query_value) {
    std::string value = query_value;
    if (query_key == "hex") {
        value = clean_hex_code(value);
    }
    return "https://www.thecolorapi.com/id?" + query_key + "=" +
           dpp::utility::url_encode(value);
}

} // namespace palette::services
