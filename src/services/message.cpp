#include "palette/services/message.hpp"
#include <cstdlib>

namespace palette::services {

std::string messages[10] = {
    "Did you know, if you type `\\` and chose Palette, then you will see all "
    "it's commands?",
    "Palette is your best friend, when it comes to choosing colors",
    "You may don't believe me, Palette can test your colors under white and "
    "black blackground and tell you if it's good!",
    "Finding complement color is very challenging, but you can do easily with "
    "Palette!",
    "Don't forget to test your chosen color to see, if it is websafe.",
    "With Palette, you can mix colors, like a real artist yk?",
    "Getting the shades and tints, then generate a palette is what Palette bot "
    "does.",
    "Getting colors information directly in discord is something that you have "
    "never seen!",
    "Palette is actually a chicken, did you know that?",
    "Chicken can mix colors with them wings. They are crazy!"};

void add_suggestion(const dpp::slashcommand_t &event) {
    int rndIdx = rand() % 10;
    auto user = event.command.get_issuing_user();
};
} // namespace palette::services
