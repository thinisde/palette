#pragma once

struct command_option_t {
    bool isPrivate;
    bool isWhitelist;
    bool requiredVote;
    int ratelimit;
};
