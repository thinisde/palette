#include "palette/services/topgg.hpp"
#include "palette/services/env_utils.hpp"

namespace palette::services {

void topgg_post_stats(dpp::cluster &bot) {

    auto headers = std::multimap<std::string, std::string>{};
    const std::string s_json =
        "{\"server_count\":" + std::to_string(dpp::get_guild_count()) + "}";

    bot.log(dpp::ll_info, s_json);

    headers.insert(
        std::pair("Authorization",
                  "Bearer " + palette::services::get_env_value("TOPGG_TOKEN")));
    headers.insert(std::pair(
        "User-Agent", "palette (https://github.com/thinisde/palette) D++"));

    bot.request(
        "https://top.gg/api/bots/stats", dpp::m_post,
        [&bot](const auto &response) {
            if (response.error != dpp::h_success) {
                bot.log(dpp::ll_error,
                        "Error message: " + std::to_string(response.error));
            } else {
                bot.log(dpp::ll_info, "Posted topgg");
            }
        },
        s_json, "application/json", headers);
}

} // namespace palette::services
