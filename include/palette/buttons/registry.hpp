#pragma once
#include "palette/services/thread_pool.hpp"
#include <dpp/dpp.h>

namespace palette::buttons {
void wire_slashcommands(dpp::cluster &bot, services::thread_pool &pool);
} // namespace palette::buttons
