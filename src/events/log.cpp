#include "palette/events/log.hpp"
#include "palette/services/env_utils.hpp"

namespace palette::events {
void wire_log(dpp::cluster &bot) {
    bot.on_log([&bot](const dpp::log_t &e) {
        std::cout << "[DPP] " << e.message << std::endl;
        if (e.severity == dpp::ll_error || e.severity == dpp::ll_critical) {
            auto webhook_url =
                palette::services::get_env_value("DISCORD_WEBHOOK");
            dpp::webhook wh(webhook_url);

            bot.execute_webhook(wh,
                                dpp::message("```\n" + e.message + "\n```"));
        }
    });
}
} // namespace palette::events
