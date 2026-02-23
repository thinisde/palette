#include "palette/events/guild.hpp"

namespace palette::events {
void wire_guild(dpp::cluster &bot) {
    bot.on_guild_create([&bot](const dpp::guild_create_t &e) {
        dpp::webhook wh(
            "https://discord.com/api/webhooks/1475475825020371048/"
            "F9S7Wb--zG7mt0UeYDPqo9gL1EVyJu3NyFwIi5ZWX3gaMAFbsrF7p4XzTnqP-"
            "Q2POGoC");

        dpp::guild guild = e.created;

        bot.execute_webhook(
            wh, dpp::message("Palette has joined guild #" +
                             std::to_string(dpp::get_guild_count()) + " **" +
                             guild.name + "**"));
    });
    bot.on_guild_delete([&bot](const dpp::guild_delete_t &e) {
        dpp::webhook wh(
            "https://discord.com/api/webhooks/1475475825020371048/"
            "F9S7Wb--zG7mt0UeYDPqo9gL1EVyJu3NyFwIi5ZWX3gaMAFbsrF7p4XzTnqP-"
            "Q2POGoC");

        dpp::guild guild = e.deleted;

        bot.execute_webhook(
            wh, dpp::message("Palette has left guild #" +
                             std::to_string(dpp::get_guild_count()) + " **" +
                             guild.name + "**"));
    });
}
} // namespace palette::events
