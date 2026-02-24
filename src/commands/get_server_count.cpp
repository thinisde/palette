#include <palette/commands/get_server_count.hpp>

namespace palette::commands {

void handle_get_server_count(dpp::cluster &_,
                             const dpp::slashcommand_t &event) {
    dpp::embed embed = dpp::embed();
    auto server_count = dpp::get_guild_count();
    embed.set_description(std::string("Palette's total server count: ") +
                          std::to_string(server_count));
    event.reply(embed);
}

} // namespace palette::commands
