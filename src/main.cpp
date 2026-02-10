#include "palette/bot.hpp"
#include "palette/services/thread_pool.hpp"
#include <dpp/dpp.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>

int main() {
    const char *token = std::getenv("DISCORD_TOKEN");
    if (!token) {
        std::cerr << "DISCORD_TOKEN not set\n";
        return 1;
    }

    const unsigned int hardware_threads = std::thread::hardware_concurrency();
    const size_t default_workers =
        std::max<size_t>(2, static_cast<size_t>(hardware_threads == 0 ? 4 : hardware_threads));
    const size_t worker_count =
        palette::services::resolve_worker_thread_count("BOT_WORKER_THREADS",
                                                       default_workers);
    palette::services::thread_pool command_pool(worker_count);

    dpp::cluster bot(token);

    std::cout << "Command worker threads: " << command_pool.size() << "\n";

    palette::wire_listeners(bot, command_pool);

    bot.start(dpp::st_wait);
    return 0;
}
