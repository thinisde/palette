#include "palette/buttons/registry.hpp"
#include "palette/buttons/shades.hpp"
#include "palette/buttons/tints.hpp"
#include "palette/services/thread_pool.hpp"

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

        if (name.rfind("shades_", 0) == 0) {
            dispatch_async(pool, bot, event, handle_shades);
            return;
        }
        if (name.rfind("tints_", 0) == 0) {
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
