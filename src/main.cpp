#include "palette/bot.hpp"
#include <dpp/dpp.h>

int main() {
    const char *token = std::getenv("DISCORD_TOKEN");
    if (!token) {
        std::cerr << "DISCORD_TOKEN not set\n";
        return 1;
    }

    dpp::cluster bot(token);

    palette::wire_listeners(bot);

    bot.start(dpp::st_wait);
    return 0;
}
