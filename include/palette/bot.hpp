#pragma once
#include "palette/services/thread_pool.hpp"
#include <dpp/dpp.h>

namespace palette {
void wire_listeners(dpp::cluster &bot, services::thread_pool &pool);
}
