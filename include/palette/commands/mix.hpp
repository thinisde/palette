#pragma once
#include "palette/types/command_options.hpp"
#include <dpp/dpp.h>

namespace palette::commands {
inline constexpr command_option_t mix_command_options{
    .isPrivate = false,
    .isWhitelist = false,
    .requiredVote = false,
    .ratelimit = 2000,
};

void handle_mix(dpp::cluster &bot, const dpp::slashcommand_t &event);
} // namespace palette::commands
