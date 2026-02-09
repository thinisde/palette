#pragma once
#include <dpp/dpp.h>
#include <functional>
#include <string>

namespace palette::services {
void fetch_color(dpp::cluster &bot, const std::string &query_key,
                 const std::string &query_value,
                 std::function<void(bool ok, std::string body)> cb);
void fetch_scheme(dpp::cluster &bot, const std::string &hex,
                  const std::string &mode, int count,
                  std::function<void(bool ok, std::string body)> cb);
std::string name_distance_label(double d);
std::string resolve_self_url(const nlohmann::json &json,
                             const std::string &fallback_url);
std::string build_id_url(const std::string &query_key,
                         const std::string &query_value);
} // namespace palette::services
