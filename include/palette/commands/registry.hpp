#pragma once
#include <dpp/dpp.h>

namespace palette::commands {
void register_commands(dpp::cluster &bot);
void wire_slashcommands(dpp::cluster &bot);
} // namespace palette::commands
