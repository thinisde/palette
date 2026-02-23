#pragma once
#include <dpp/dpp.h>
#include <topgg/topgg.h>

namespace palette::events {
void wire_ready(dpp::cluster &bot, topgg::client &topgg);
}
