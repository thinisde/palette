#include "palette/bot.hpp"
#include "palette/buttons/registry.hpp"
#include "palette/commands/registry.hpp"
#include "palette/events/guild.hpp"
#include "palette/events/log.hpp"
#include "palette/events/ready.hpp"

namespace palette {
void wire_listeners(dpp::cluster &bot, services::thread_pool &pool) {
    events::wire_ready(bot);
    events::wire_log(bot);
    events::wire_guild(bot);
    commands::wire_slashcommands(bot, pool);
    buttons::wire_buttons(bot, pool);
}
} // namespace palette
