#include "palette/commands/get_version.hpp"

namespace palette::commands {

static inline void trim_inplace(std::string &s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

std::optional<std::string> get_deployed_tag(
    const std::string &path = "/var/lib/palette-deploy-agent/current_tag") {
    std::ifstream in(path);
    if (!in.is_open())
        return std::nullopt;

    std::string tag;
    std::getline(in, tag);
    trim_inplace(tag);

    if (tag.empty())
        return std::nullopt;
    return tag; // e.g. "v1.2.3"
}

void handle_get_version(dpp::cluster &_, const dpp::slashcommand_t &event) {
    dpp::embed embed = dpp::embed();
    std::optional<std::string> version = get_deployed_tag();
    if (version) {
        embed.set_description(std::string("Palette's current version: ") +
                              version->data());
    } else if (version == std::nullopt) {
        embed.set_description("No version found!");
    }
    event.reply(embed);
}

} // namespace palette::commands
