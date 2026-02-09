#include "palette/events/ready.hpp"
#include "palette/commands/registry.hpp"

namespace palette::events {
void wire_ready(dpp::cluster &bot) {
    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_commands_once>()) {
            commands::register_commands(bot); // overwrite commands
        }
    });
}
} // namespace palette::events
