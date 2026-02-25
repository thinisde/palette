#include "palette/events/ready.hpp"
#include "palette/commands/registry.hpp"
#include "palette/services/env_utils.hpp"

namespace palette::events {
void wire_ready(dpp::cluster &bot, topgg::client &topgg) {
    bot.on_ready([&bot, &topgg](const dpp::ready_t &_) {
        const bool production = palette::services::is_production_environment();

        if (production) {
            // topgg.start_autoposter();
        }

        if (dpp::run_once<struct register_commands_once>()) {
            commands::register_commands(bot); // overwrite commands
        }
    });
}
} // namespace palette::events
