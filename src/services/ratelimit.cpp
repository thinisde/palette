#include "palette/services/ratelimit.hpp"
#include <chrono>
#include <unordered_map>

namespace palette::services {
static std::unordered_map<dpp::snowflake, std::chrono::system_clock::time_point>
    ratelimits;

bool is_ratelimited(dpp::snowflake user, int64_t ms) {
    auto now = std::chrono::system_clock::now();

    auto &last = ratelimits[user];

    if (last.time_since_epoch().count() == 0) {
        last = now;
        return false;
    }

    if ((now - last) < std::chrono::milliseconds(ms))
        return true;

    last = now;
    return false;
}
} // namespace palette::services
