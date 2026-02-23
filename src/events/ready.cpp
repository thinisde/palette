#include "palette/events/ready.hpp"
#include "palette/commands/registry.hpp"
#include "palette/services/env_utils.hpp"
// #include "palette/services/topgg.hpp"
// #include <functional> // std::ref
// #include <thread>

namespace palette::events {
void wire_ready(dpp::cluster &bot) {
    bot.on_ready([&bot](const dpp::ready_t &event) {
        const bool production = palette::services::is_production_environment();

        // TODO returns 400 Bad Requests
        // if (!production) {
        //     std::cout << "[CRON] starting..." << std::endl;
        //     static std::thread cron_thread(palette::services::task,
        //                                    std::ref(bot));
        //     cron_thread.detach();
        // }

        if (dpp::run_once<struct register_commands_once>()) {
            commands::register_commands(bot); // overwrite commands
        }
    });
}
} // namespace palette::events
