#pragma once
#include "palette/types/command_options.hpp"
#include <dpp/dpp.h>

namespace palette::commands {
inline constexpr command_option_t websafe_command_options{
    .isPrivate = false,
    .isWhitelist = false,
    .requiredVote = false,
    .ratelimit = 500,
};

void handle_websafe(dpp::cluster &bot, const dpp::slashcommand_t &event);
} // namespace palette::commands
