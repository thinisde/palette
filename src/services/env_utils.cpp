#include "palette/services/env_utils.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>

namespace palette::services {
namespace {
std::string trim_copy(const std::string &value) {
    size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(begin, end - begin);
}

void set_env_if_missing(const std::string &key, const std::string &value) {
    if (key.empty() || std::getenv(key.c_str()) != nullptr) {
        return;
    }

#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 0);
#endif
}
} // namespace

void load_dotenv_file(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string value = trim_copy(line);
        if (value.empty() || value.front() == '#') {
            continue;
        }

        const std::string export_prefix = "export ";
        if (value.rfind(export_prefix, 0) == 0) {
            value.erase(0, export_prefix.size());
            value = trim_copy(value);
        }

        const size_t equals = value.find('=');
        if (equals == std::string::npos) {
            continue;
        }

        const std::string key = trim_copy(value.substr(0, equals));
        std::string raw_value = trim_copy(value.substr(equals + 1));
        if (raw_value.size() >= 2 &&
            ((raw_value.front() == '"' && raw_value.back() == '"') ||
             (raw_value.front() == '\'' && raw_value.back() == '\''))) {
            raw_value = raw_value.substr(1, raw_value.size() - 2);
        }

        set_env_if_missing(key, raw_value);
    }
}

std::string get_env_value(std::string_view key) {
    const std::string key_string(key);
    const char *raw = std::getenv(key_string.c_str());
    return raw ? std::string(raw) : std::string();
}

std::string get_env_value_or(std::string_view key, std::string_view fallback) {
    std::string value = get_env_value(key);
    return value.empty() ? std::string(fallback) : value;
}

std::optional<uint64_t> get_env_u64(std::string_view key) {
    const std::string raw = get_env_value(key);
    if (raw.empty()) {
        return std::nullopt;
    }

    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(raw.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return std::nullopt;
    }
    return static_cast<uint64_t>(parsed);
}

std::string normalize_ascii_lower(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool is_production_environment() {
    std::string env = get_env_value("BOT_ENV");
    if (env.empty()) {
        env = get_env_value("APP_ENV");
    }
    if (env.empty()) {
        env = get_env_value("ENV");
    }

    env = normalize_ascii_lower(trim_copy(env));
    return env == "production" || env == "prod";
}

std::string resolve_discord_token() {
    const bool production = is_production_environment();

    if (production) {
        std::string token = get_env_value("DISCORD_TOKEN_PRODUCTION");
        if (!token.empty()) {
            return token;
        }
    } else {
        std::string token = get_env_value("DISCORD_TOKEN_DEVELOPMENT");
        if (!token.empty()) {
            return token;
        }
    }

    return get_env_value("DISCORD_TOKEN");
}

} // namespace palette::services
