#pragma once
#include "palette/types/command_options.hpp"
#include <dpp/dpp.h>

namespace palette::commands {
inline constexpr command_option_t tints_command_options{
    .isPrivate = false,
    .isWhitelist = false,
    .requiredVote = true,
    .ratelimit = 5000,
};

void handle_tints(dpp::cluster &bot, const dpp::slashcommand_t &event);
} // namespace palette::commands
