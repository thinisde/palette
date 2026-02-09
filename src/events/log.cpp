#include "palette/events/log.hpp"

namespace palette::events {
void wire_log(dpp::cluster &bot) {
    bot.on_log([](const dpp::log_t &e) {
        std::cout << "[DPP] " << e.message << std::endl;
    });
}
} // namespace palette::events
