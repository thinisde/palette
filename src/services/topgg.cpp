#include "palette/services/topgg.hpp"
#include "palette/services/env_utils.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

namespace palette::services {

void topgg_post_stats(dpp::cluster &bot) {
    std::string topgg_token = palette::services::get_env_value("TOPGG_TOKEN");

    std::string body =
        "{\"server_count\":" + std::to_string(dpp::get_guild_count()) + "}";

    std::multimap<std::string, std::string> headers = {
        {"Authorization", topgg_token}, {"Accept", "application/json"}};

    std::string bot_id = bot.me.id.str();

    bot.request(
        "https://top.gg/api/bots/1470481920100925683/stats", dpp::m_post,
        [&bot](const dpp::http_request_completion_t &r) {
            bot.log(dpp::ll_info, "Status: " + std::to_string(r.status) + "\n" +
                                      r.body + "\n");
        },
        body, "application/json", headers, "1.1");
}

void task(dpp::cluster &bot) {
    std::atomic<bool> running = true;
    topgg_post_stats(bot);
    while (running) {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm local = *std::localtime(&tt);

        if (local.tm_hour == 3 && local.tm_min == 0) {
            topgg_post_stats(bot);
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

} // namespace palette::services
