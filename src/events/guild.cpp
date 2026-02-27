#include "palette/events/guild.hpp"
#include "palette/services/env_utils.hpp"

namespace palette::events {
void wire_guild(dpp::cluster &bot) {
    auto mode = palette::services::is_production_environment();
    if (!mode)
        return;

    auto webhook_url = palette::services::get_env_value("DISCORD_WEBHOOK");

    bot.on_guild_create([&bot, &webhook_url](const dpp::guild_create_t &e) {
        dpp::webhook wh(webhook_url);

        dpp::guild guild = e.created;

        bot.execute_webhook(
            wh, dpp::message(bot.me.global_name + " has joined guild #" +
                             std::to_string(dpp::get_guild_count()) + " **" +
                             guild.name + "**"));
    });
    bot.on_guild_delete([&bot, &webhook_url](const dpp::guild_delete_t &e) {
        dpp::webhook wh(webhook_url);

        dpp::guild guild = e.deleted;

        bot.execute_webhook(
            wh, dpp::message(bot.me.global_name + " has left guild #" +
                             std::to_string(dpp::get_guild_count()) + " **" +
                             guild.name + "**"));
    });
}
} // namespace palette::events
