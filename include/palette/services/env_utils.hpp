#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace palette::services {

void load_dotenv_file(const std::string &path = ".env");
std::string get_env_value(std::string_view key);
std::string get_env_value_or(std::string_view key, std::string_view fallback);
std::optional<uint64_t> get_env_u64(std::string_view key);
std::string normalize_ascii_lower(std::string value);
bool is_production_environment();
std::string resolve_discord_token();

} // namespace palette::services
