#include "palette/bot.hpp"
#include "palette/services/env_utils.hpp"
#include "palette/services/thread_pool.hpp"
#include <algorithm>
#include <cstdlib>
#include <dpp/dpp.h>
#include <iostream>
#include <string>
#include <thread>

int main() {
    palette::services::load_dotenv_file();

    const bool production = palette::services::is_production_environment();
    const std::string token = palette::services::resolve_discord_token();
    if (token.empty()) {
        std::cerr << "Discord token not set. Use "
                     "`DISCORD_TOKEN_DEVELOPMENT`/`DISCORD_TOKEN_PRODUCTION` "
                     "or fallback `DISCORD_TOKEN`.\n";
        return 1;
    }

    const unsigned int hardware_threads = std::thread::hardware_concurrency();
    const size_t default_workers = std::max<size_t>(
        2, static_cast<size_t>(hardware_threads == 0 ? 4 : hardware_threads));
    const size_t worker_count = palette::services::resolve_worker_thread_count(
        "BOT_WORKER_THREADS", default_workers);
    dpp::cluster bot(token);
    palette::services::thread_pool command_pool(worker_count);

    std::cout << "Command worker threads: " << command_pool.size() << "\n";
    std::cout << "Environment: " << (production ? "production" : "development")
              << "\n";

    palette::wire_listeners(bot, command_pool);

    bot.start(dpp::st_wait);
    return 0;
}
