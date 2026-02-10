#include "palette/bot.hpp"
#include "palette/commands/registry.hpp"
#include "palette/events/log.hpp"
#include "palette/events/ready.hpp"

namespace palette {
void wire_listeners(dpp::cluster &bot, services::thread_pool &pool) {
    events::wire_ready(bot);
    events::wire_log(bot);
    commands::wire_slashcommands(bot, pool);
}
} // namespace palette
