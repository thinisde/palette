#pragma once
#include "palette/types/command_options.hpp"
#include <dpp/dpp.h>

namespace palette::commands {
inline constexpr command_option_t get_version_command_options{
    .isPrivate = true,
    .isWhitelist = false,
    .requiredVote = false,
    .ratelimit = 0,
};

void handle_get_version(dpp::cluster &bot, const dpp::slashcommand_t &event);
}
