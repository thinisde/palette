#pragma once
#include "dpp/dpp.h"
#include <ctime>

namespace palette::services {
bool is_ratelimited(dpp::snowflake user_id, int64_t ms);
} // namespace palette::services
