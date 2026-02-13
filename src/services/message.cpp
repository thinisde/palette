#include "palette/services/message.hpp"
#include <cstddef>
#include <random>

std::size_t random_index(std::size_t n) {
    thread_local std::mt19937 gen{std::random_device{}()};
    thread_local std::uniform_int_distribution<std::size_t> dist(0, n - 1);
    return dist(gen);
}

namespace palette::services {

std::string messages[10] = {
    "Did you know? If you type `\\` and select Palette, you can see all "
    "available commands.",
    "Palette is your best friend when it comes to choosing the perfect color.",
    "You might not believe it, but Palette can test your colors on both white "
    "and black backgrounds to check readability.",
    "Finding a complementary color can be tricky — Palette makes it "
    "effortless.",
    "Don’t forget to test your chosen color to see if it’s web-safe.",
    "With Palette, you can mix colors like a real artist.",
    "Generate shades and tints instantly — Palette builds beautiful palettes "
    "for you.",
    "Get detailed color information directly inside Discord — no external "
    "tools needed.",
    "Fun fact: Palette might secretly be a chicken.",
    "Even chickens can mix colors with their wings. Wild, right?"};

void add_suggestion(const dpp::slashcommand_t &event) {
    auto rndIdx = random_index(std::size(messages));

    const std::string suggestion = messages[rndIdx];

    event.get_original_response(
        [event, suggestion](const dpp::confirmation_callback_t &cc) {
            if (cc.is_error()) {
                std::cerr << "get_original_response failed: "
                          << cc.get_error().message << "\n";
                return;
            }

            // On success, cc.value holds a dpp::message
            const auto &msg = std::get<dpp::message>(cc.value);
            dpp::message edited = msg;
            auto embed = dpp::embed().set_description(suggestion);
            edited.embeds.push_back(embed);

            event.edit_original_response(edited);
        });
};
} // namespace palette::services
