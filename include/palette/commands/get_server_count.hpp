#pragma once
#include <dpp/dpp.h>

namespace palette::commands {
void handle_get_server_count(dpp::cluster &bot,
                             const dpp::slashcommand_t &event);
}
