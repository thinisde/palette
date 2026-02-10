#include "palette/commands/registry.hpp"
#include "palette/commands/blacktest.hpp"
#include "palette/commands/color.hpp"
#include "palette/commands/complementary.hpp"
#include "palette/commands/scheme.hpp"
#include "palette/commands/shades.hpp"
#include "palette/commands/splitcomplementary.hpp"
#include "palette/commands/tints.hpp"
#include "palette/commands/websafe.hpp"
#include "palette/commands/whitetest.hpp"
#include "palette/services/env_utils.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace palette::commands {
namespace {
using command_handler =
    std::function<void(dpp::cluster &, const dpp::slashcommand_t &)>;

void dispatch_async(services::thread_pool &pool, dpp::cluster &bot,
                    const dpp::slashcommand_t &event, command_handler handler) {
    const dpp::slashcommand_t event_copy = event;
    pool.enqueue([event_copy, &bot, handler = std::move(handler)]() {
        handler(bot, event_copy);
    });
}

std::optional<dpp::snowflake> resolve_guild_id_for_registration() {
    if (const auto id = services::get_env_u64("DISCORD_DEV_GUILD_ID")) {
        return dpp::snowflake(*id);
    }
    if (const auto id = services::get_env_u64("DISCORD_GUILD_ID")) {
        return dpp::snowflake(*id);
    }
    return std::nullopt;
}

void register_by_environment(dpp::cluster &bot,
                             const std::vector<dpp::slashcommand> &commands) {
    if (services::is_production_environment()) {
        bot.log(dpp::ll_info,
                "Registering slash commands globally (production mode).");
        bot.global_bulk_command_create(commands);
        return;
    }

    const auto guild_id = resolve_guild_id_for_registration();
    if (guild_id.has_value()) {
        bot.log(dpp::ll_info, "Registering slash commands to guild " +
                                  std::to_string(static_cast<uint64_t>(*guild_id)) +
                                  " (development mode).");
        bot.guild_bulk_command_create(commands, *guild_id);
        return;
    }

    bot.log(dpp::ll_warning,
            "Development mode detected but no DISCORD_DEV_GUILD_ID/"
            "DISCORD_GUILD_ID provided. Falling back to global registration.");
    bot.global_bulk_command_create(commands);
}
} // namespace

void register_commands(dpp::cluster &bot) {
    dpp::slashcommand color("color", "Identify a color via hex/rgb/hsl/cmyk",
                            bot.me.id);
    color.add_option(dpp::command_option(dpp::co_string, "hex",
                                         "Hex like 24B1E0 or #24B1E0", false));
    color.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    color.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    color.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    dpp::slashcommand complementary("complementary",
                                    "Find the complementary color", bot.me.id);
    complementary.add_option(dpp::command_option(
        dpp::co_string, "hex", "Hex like 24B1E0 or #24B1E0", false));
    complementary.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    complementary.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    complementary.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    dpp::slashcommand scheme("scheme", "Generate a color scheme from a seed",
                             bot.me.id);
    scheme.add_option(dpp::command_option(dpp::co_string, "hex",
                                          "Hex like 24B1E0 or #24B1E0", true));

    dpp::command_option mode(dpp::co_string, "mode",
                             "Scheme mode (default: monochrome)", false);
    mode.add_choice(dpp::command_option_choice("monochrome", "monochrome"));
    mode.add_choice(
        dpp::command_option_choice("monochrome-dark", "monochrome-dark"));
    mode.add_choice(
        dpp::command_option_choice("monochrome-light", "monochrome-light"));
    mode.add_choice(dpp::command_option_choice("analogic", "analogic"));
    mode.add_choice(dpp::command_option_choice("complement", "complement"));
    mode.add_choice(dpp::command_option_choice("analogic-complement",
                                               "analogic-complement"));
    mode.add_choice(dpp::command_option_choice("triad", "triad"));
    mode.add_choice(dpp::command_option_choice("quad", "quad"));
    scheme.add_option(mode);

    scheme.add_option(dpp::command_option(
        dpp::co_integer, "count", "Number of colors to return (1-20)", false));

    dpp::slashcommand shades(
        "shades", "Generate numbered shades image (toward black)", bot.me.id);
    shades.add_option(dpp::command_option(
        dpp::co_integer, "amount", "Number of shades per color (2-8)", true));
    shades.add_option(dpp::command_option(
        dpp::co_string, "hex",
        "Hex list separated by ';' (example: #FF0000; 00AAFF)", false));
    shades.add_option(dpp::command_option(
        dpp::co_string, "rgb",
        "RGB list separated by ';' (example: 255,0,0; rgb(0,128,255))", false));
    shades.add_option(dpp::command_option(
        dpp::co_string, "hsl",
        "HSL list separated by ';' (example: 0,100%,50%; hsl(210,100%,50%))",
        false));
    shades.add_option(dpp::command_option(
        dpp::co_string, "cmyk",
        "CMYK list separated by ';' (example: 0,100,100,0; cmyk(100,0,0,0))",
        false));

    dpp::slashcommand tints(
        "tints", "Generate numbered tints image (toward white)", bot.me.id);
    tints.add_option(dpp::command_option(
        dpp::co_integer, "amount", "Number of tints per color (2-8)", true));
    tints.add_option(dpp::command_option(
        dpp::co_string, "hex",
        "Hex list separated by ';' (example: #FF0000; 00AAFF)", false));
    tints.add_option(dpp::command_option(
        dpp::co_string, "rgb",
        "RGB list separated by ';' (example: 255,0,0; rgb(0,128,255))",
        false));
    tints.add_option(dpp::command_option(
        dpp::co_string, "hsl",
        "HSL list separated by ';' (example: 0,100%,50%; hsl(210,100%,50%))",
        false));
    tints.add_option(dpp::command_option(
        dpp::co_string, "cmyk",
        "CMYK list separated by ';' (example: 0,100,100,0; cmyk(100,0,0,0))",
        false));

    dpp::slashcommand splitcomplementary(
        "splitcomplementary",
        "Generate a split complementary scheme (3 colors)", bot.me.id);
    splitcomplementary.add_option(dpp::command_option(
        dpp::co_string, "hex", "Hex like 24B1E0 or #24B1E0", false));
    splitcomplementary.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    splitcomplementary.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    splitcomplementary.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    dpp::slashcommand websafe(
        "websafe", "Compare a color with its nearest web safe color",
        bot.me.id);
    websafe.add_option(dpp::command_option(
        dpp::co_string, "hex", "Hex like 24B1E0 or #24B1E0", false));
    websafe.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    websafe.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    websafe.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    dpp::slashcommand blacktest(
        "blacktest", "WCAG contrast test of a color on black background",
        bot.me.id);
    blacktest.add_option(dpp::command_option(
        dpp::co_string, "hex", "Hex like 24B1E0 or #24B1E0", false));
    blacktest.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    blacktest.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    blacktest.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    dpp::slashcommand whitetest(
        "whitetest", "WCAG contrast test of a color on white background",
        bot.me.id);
    whitetest.add_option(dpp::command_option(
        dpp::co_string, "hex", "Hex like 24B1E0 or #24B1E0", false));
    whitetest.add_option(dpp::command_option(
        dpp::co_string, "rgb", "RGB like 0,71,171 or rgb(0,71,171)", false));
    whitetest.add_option(dpp::command_option(
        dpp::co_string, "hsl", "HSL like 215,100%,34% or hsl(...)", false));
    whitetest.add_option(dpp::command_option(
        dpp::co_string, "cmyk", "CMYK like 100,58,0,33 or cmyk(...)", false));

    const std::vector<dpp::slashcommand> commands = {
        color, complementary, scheme, shades, tints, splitcomplementary,
        websafe, blacktest, whitetest};

    register_by_environment(bot, commands);
}

void wire_slashcommands(dpp::cluster &bot, services::thread_pool &pool) {
    bot.on_slashcommand([&bot, &pool](const dpp::slashcommand_t &event) {
        const auto &name = event.command.get_command_name();

        if (name == "color") {
            dispatch_async(pool, bot, event, handle_color);
            return;
        }
        if (name == "scheme") {
            dispatch_async(pool, bot, event, handle_scheme);
            return;
        }
        if (name == "complementary") {
            dispatch_async(pool, bot, event, handle_complementary);
            return;
        }
        if (name == "shades") {
            dispatch_async(pool, bot, event, handle_shades);
            return;
        }
        if (name == "tints") {
            dispatch_async(pool, bot, event, handle_tints);
            return;
        }
        if (name == "splitcomplementary") {
            dispatch_async(pool, bot, event, handle_splitcomplementary);
            return;
        }
        if (name == "websafe") {
            dispatch_async(pool, bot, event, handle_websafe);
            return;
        }
        if (name == "blacktest") {
            dispatch_async(pool, bot, event, handle_blacktest);
            return;
        }
        if (name == "whitetest") {
            dispatch_async(pool, bot, event, handle_whitetest);
            return;
        }
        // default fallback
        dispatch_async(
            pool, bot, event,
            [](dpp::cluster &, const dpp::slashcommand_t &unknown_event) {
                unknown_event.reply("Unknown command.");
            });
    });
}

} // namespace palette::commands
