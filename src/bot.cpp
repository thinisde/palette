#include "palette/bot.hpp"
#include "palette/buttons/registry.hpp"
#include "palette/commands/registry.hpp"
#include "palette/events/guild.hpp"
#include "palette/events/log.hpp"
#include "palette/events/ready.hpp"
#include "palette/services/env_utils.hpp"
#include <topgg/topgg.h>

namespace palette {
void wire_listeners(dpp::cluster &bot, services::thread_pool &pool) {

    topgg::client topgg_client{bot,
                               palette::services::get_env_value("TOPGG_TOKEN")};

    events::wire_ready(bot, topgg_client);
    events::wire_log(bot);
    events::wire_guild(bot);
    commands::wire_slashcommands(bot, pool);
    buttons::wire_buttons(bot, pool);
}
} // namespace palette
