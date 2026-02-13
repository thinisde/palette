#include "palette/buttons/registry.hpp"
#include "palette/buttons/shades.hpp"
#include "palette/buttons/tints.hpp"
#include "palette/services/thread_pool.hpp"
#include <string_view>
#include <utility>

inline std::pair<std::string_view, std::string_view>
split_custom_id(std::string_view custom_id) {
    const auto pos = custom_id.find(':');
    if (pos == std::string_view::npos) {
        return {custom_id, std::string_view{}};
    }
    return {custom_id.substr(0, pos), custom_id.substr(pos + 1)};
}

namespace palette::buttons {
namespace {
using command_handler =
    std::function<void(dpp::cluster &, const dpp::button_click_t &)>;

void dispatch_async(services::thread_pool &pool, dpp::cluster &bot,
                    const dpp::button_click_t &event, command_handler handler) {
    const dpp::button_click_t event_copy = event;
    pool.enqueue([event_copy, &bot, handler = std::move(handler)]() {
        handler(bot, event_copy);
    });
}
} // namespace

void wire_slashcommands(dpp::cluster &bot, services::thread_pool &pool) {
    bot.on_button_click([&bot, &pool](const dpp::button_click_t &event) {
        const auto &name = event.custom_id;

        if (name == "shades_next") {
            dispatch_async(pool, bot, event, handle_shades);
            return;
        }

        if (name == "shades_back") {
            dispatch_async(pool, bot, event, handle_shades);
            return;
        }
        if (name == "tints_next") {
            dispatch_async(pool, bot, event, handle_tints);
            return;
        }
        if (name == "tints_back") {
            dispatch_async(pool, bot, event, handle_tints);
            return;
        }

        dispatch_async(
            pool, bot, event,
            [](dpp::cluster &, const dpp::button_click_t &unknown_event) {
                unknown_event.reply("Unknown button event.");
            });
    });
}

} // namespace palette::buttons
