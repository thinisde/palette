#include "palette/services/color_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>

namespace palette::services {
namespace {
int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

bool parse_number(std::string_view token, double &out) {
    const std::string trimmed = trim_copy(token);
    if (trimmed.empty()) {
        return false;
    }

    size_t consumed = 0;
    try {
        out = std::stod(trimmed, &consumed);
    } catch (...) {
        return false;
    }
    return consumed == trimmed.size();
}

bool parse_channel(std::string_view token, uint8_t &out) {
    double value = 0.0;
    if (!parse_number(token, value)) {
        return false;
    }

    const int rounded = static_cast<int>(std::round(value));
    if (rounded < 0 || rounded > 255) {
        return false;
    }

    out = static_cast<uint8_t>(rounded);
    return true;
}

bool parse_percent(std::string token, double &out) {
    token = trim_copy(token);
    if (token.empty()) {
        return false;
    }

    if (!token.empty() && token.back() == '%') {
        token.pop_back();
    }

    double value = 0.0;
    if (!parse_number(token, value)) {
        return false;
    }
    if (value < 0.0 || value > 100.0) {
        return false;
    }

    out = value;
    return true;
}

bool split_triplet(const std::string &value, std::string &a, std::string &b,
                   std::string &c) {
    const size_t first = value.find(',');
    if (first == std::string::npos) {
        return false;
    }
    const size_t second = value.find(',', first + 1);
    if (second == std::string::npos) {
        return false;
    }
    if (value.find(',', second + 1) != std::string::npos) {
        return false;
    }

    a = value.substr(0, first);
    b = value.substr(first + 1, second - first - 1);
    c = value.substr(second + 1);
    return true;
}

bool split_quad(const std::string &value, std::string &a, std::string &b,
                std::string &c, std::string &d) {
    const size_t first = value.find(',');
    if (first == std::string::npos) {
        return false;
    }
    const size_t second = value.find(',', first + 1);
    if (second == std::string::npos) {
        return false;
    }
    const size_t third = value.find(',', second + 1);
    if (third == std::string::npos) {
        return false;
    }
    if (value.find(',', third + 1) != std::string::npos) {
        return false;
    }

    a = value.substr(0, first);
    b = value.substr(first + 1, second - first - 1);
    c = value.substr(second + 1, third - second - 1);
    d = value.substr(third + 1);
    return true;
}

std::string normalize_wrapper(std::string value, const char *prefix) {
    std::string lower = value;
    std::transform(
        lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string expected_prefix = std::string(prefix) + "(";
    if (lower.rfind(expected_prefix, 0) == 0 && value.size() > expected_prefix.size() &&
        value.back() == ')') {
        return value.substr(expected_prefix.size(), value.size() - expected_prefix.size() - 1);
    }
    return value;
}

std::string to_hex_pair(int value) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out(2, '0');
    out[0] = digits[(value >> 4) & 0xF];
    out[1] = digits[value & 0xF];
    return out;
}

int nearest_web_safe_channel(int value) {
    static constexpr int kWebSafeChannels[6] = {0, 51, 102, 153, 204, 255};
    int nearest = kWebSafeChannels[0];
    int best_distance = std::abs(value - nearest);

    for (int i = 1; i < 6; ++i) {
        const int candidate = kWebSafeChannels[i];
        const int distance = std::abs(value - candidate);
        if (distance < best_distance) {
            best_distance = distance;
            nearest = candidate;
        }
    }
    return nearest;
}

bool is_web_safe_channel(int value) {
    return value == 0 || value == 51 || value == 102 || value == 153 ||
           value == 204 || value == 255;
}

double srgb_channel_to_linear(uint8_t channel) {
    const double v = static_cast<double>(channel) / 255.0;
    if (v <= 0.03928) {
        return v / 12.92;
    }
    return std::pow((v + 0.055) / 1.055, 2.4);
}
} // namespace

bool read_optional_string(const dpp::command_value &value, std::string &out) {
    if (const auto *p = std::get_if<std::string>(&value); p && !p->empty()) {
        out = *p;
        return true;
    }
    return false;
}

single_color_input_result
parse_single_color_input(const dpp::slashcommand_t &event) {
    single_color_input_result result;

    std::string hex;
    std::string rgb;
    std::string hsl;
    std::string cmyk;

    const bool has_hex = read_optional_string(event.get_parameter("hex"), hex);
    const bool has_rgb = read_optional_string(event.get_parameter("rgb"), rgb);
    const bool has_hsl = read_optional_string(event.get_parameter("hsl"), hsl);
    const bool has_cmyk =
        read_optional_string(event.get_parameter("cmyk"), cmyk);

    const int provided = static_cast<int>(has_hex) + static_cast<int>(has_rgb) +
                         static_cast<int>(has_hsl) + static_cast<int>(has_cmyk);
    if (provided == 0) {
        result.error = "Provide one of `hex`, `rgb`, `hsl`, or `cmyk`.";
        return result;
    }
    if (provided > 1) {
        result.error =
            "Provide only one input: `hex`, `rgb`, `hsl`, or `cmyk`.";
        return result;
    }

    if (has_hex) {
        result.query_key = "hex";
        result.query_value = hex;
    } else if (has_rgb) {
        result.query_key = "rgb";
        result.query_value = rgb;
    } else if (has_hsl) {
        result.query_key = "hsl";
        result.query_value = hsl;
    } else {
        result.query_key = "cmyk";
        result.query_value = cmyk;
    }

    result.ok = true;
    return result;
}

multi_color_input_result
parse_multi_color_input(const dpp::slashcommand_t &event, size_t max_colors) {
    multi_color_input_result result;

    std::string hex;
    std::string rgb;
    std::string hsl;
    std::string cmyk;

    const bool has_hex = read_optional_string(event.get_parameter("hex"), hex);
    const bool has_rgb = read_optional_string(event.get_parameter("rgb"), rgb);
    const bool has_hsl = read_optional_string(event.get_parameter("hsl"), hsl);
    const bool has_cmyk =
        read_optional_string(event.get_parameter("cmyk"), cmyk);

    if (!has_hex && !has_rgb && !has_hsl && !has_cmyk) {
        result.error =
            "Provide at least one color via `hex`, `rgb`, `hsl`, or `cmyk`.";
        return result;
    }

    auto append_values = [&result](const std::string &raw, color_model model,
                                   const char *key) -> bool {
        if (raw.empty()) {
            return true;
        }
        if (!parse_color_list(raw, model, result.colors)) {
            result.error = std::string("Invalid `") + key +
                           "` list. Use ';' to separate multiple values.";
            return false;
        }
        return true;
    };

    if (!append_values(hex, color_model::hex, "hex") ||
        !append_values(rgb, color_model::rgb, "rgb") ||
        !append_values(hsl, color_model::hsl, "hsl") ||
        !append_values(cmyk, color_model::cmyk, "cmyk")) {
        return result;
    }

    if (result.colors.size() > max_colors) {
        result.error =
            "Provide at most " + std::to_string(max_colors) + " source colors.";
        return result;
    }

    result.ok = true;
    return result;
}

std::string trim_copy(std::string_view value) {
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

    return std::string(value.substr(begin, end - begin));
}

bool parse_hex_to_rgb(const std::string &value, rgb_color &out) {
    std::string hex = trim_copy(value);
    if (!hex.empty() && hex.front() == '#') {
        hex.erase(hex.begin());
    }

    if (hex.size() == 3) {
        std::string expanded;
        expanded.reserve(6);
        for (const char c : hex) {
            expanded.push_back(c);
            expanded.push_back(c);
        }
        hex = std::move(expanded);
    }

    if (hex.size() != 6) {
        return false;
    }

    const int r_hi = hex_char_to_int(hex[0]);
    const int r_lo = hex_char_to_int(hex[1]);
    const int g_hi = hex_char_to_int(hex[2]);
    const int g_lo = hex_char_to_int(hex[3]);
    const int b_hi = hex_char_to_int(hex[4]);
    const int b_lo = hex_char_to_int(hex[5]);
    if (r_hi < 0 || r_lo < 0 || g_hi < 0 || g_lo < 0 || b_hi < 0 ||
        b_lo < 0) {
        return false;
    }

    out.r = static_cast<uint8_t>((r_hi << 4) | r_lo);
    out.g = static_cast<uint8_t>((g_hi << 4) | g_lo);
    out.b = static_cast<uint8_t>((b_hi << 4) | b_lo);
    return true;
}

bool parse_rgb_to_rgb(const std::string &value, rgb_color &out) {
    const std::string body = normalize_wrapper(trim_copy(value), "rgb");

    std::string r_token;
    std::string g_token;
    std::string b_token;
    if (!split_triplet(body, r_token, g_token, b_token)) {
        return false;
    }

    return parse_channel(r_token, out.r) && parse_channel(g_token, out.g) &&
           parse_channel(b_token, out.b);
}

bool hsl_to_rgb(double h, double s, double l, rgb_color &out) {
    if (s < 0.0 || s > 100.0 || l < 0.0 || l > 100.0) {
        return false;
    }

    h = std::fmod(h, 360.0);
    if (h < 0.0) {
        h += 360.0;
    }
    const double hd = h / 360.0;
    const double sd = s / 100.0;
    const double ld = l / 100.0;

    double rd = ld;
    double gd = ld;
    double bd = ld;

    if (sd > 0.0) {
        const double q = ld < 0.5 ? ld * (1.0 + sd) : ld + sd - ld * sd;
        const double p = 2.0 * ld - q;
        auto hue2rgb = [](double pp, double qq, double t) {
            if (t < 0.0) {
                t += 1.0;
            }
            if (t > 1.0) {
                t -= 1.0;
            }
            if (t < 1.0 / 6.0) {
                return pp + (qq - pp) * 6.0 * t;
            }
            if (t < 1.0 / 2.0) {
                return qq;
            }
            if (t < 2.0 / 3.0) {
                return pp + (qq - pp) * (2.0 / 3.0 - t) * 6.0;
            }
            return pp;
        };

        rd = hue2rgb(p, q, hd + 1.0 / 3.0);
        gd = hue2rgb(p, q, hd);
        bd = hue2rgb(p, q, hd - 1.0 / 3.0);
    }

    out.r =
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(rd * 255.0)),
                                        0, 255));
    out.g =
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(gd * 255.0)),
                                        0, 255));
    out.b =
        static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(bd * 255.0)),
                                        0, 255));
    return true;
}

bool parse_hsl_to_rgb(const std::string &value, rgb_color &out) {
    const std::string body = normalize_wrapper(trim_copy(value), "hsl");
    std::string h_token;
    std::string s_token;
    std::string l_token;
    if (!split_triplet(body, h_token, s_token, l_token)) {
        return false;
    }

    double h = 0.0;
    double s = 0.0;
    double l = 0.0;
    if (!parse_number(h_token, h) || !parse_percent(s_token, s) ||
        !parse_percent(l_token, l)) {
        return false;
    }

    return hsl_to_rgb(h, s, l, out);
}

bool parse_cmyk_to_rgb(const std::string &value, rgb_color &out) {
    const std::string body = normalize_wrapper(trim_copy(value), "cmyk");
    std::string c_token;
    std::string m_token;
    std::string y_token;
    std::string k_token;
    if (!split_quad(body, c_token, m_token, y_token, k_token)) {
        return false;
    }

    double c = 0.0;
    double m = 0.0;
    double y = 0.0;
    double k = 0.0;
    if (!parse_percent(c_token, c) || !parse_percent(m_token, m) ||
        !parse_percent(y_token, y) || !parse_percent(k_token, k)) {
        return false;
    }

    const double c01 = c / 100.0;
    const double m01 = m / 100.0;
    const double y01 = y / 100.0;
    const double k01 = k / 100.0;

    out.r = static_cast<uint8_t>(std::clamp(
        static_cast<int>(std::round(255.0 * (1.0 - c01) * (1.0 - k01))), 0,
        255));
    out.g = static_cast<uint8_t>(std::clamp(
        static_cast<int>(std::round(255.0 * (1.0 - m01) * (1.0 - k01))), 0,
        255));
    out.b = static_cast<uint8_t>(std::clamp(
        static_cast<int>(std::round(255.0 * (1.0 - y01) * (1.0 - k01))), 0,
        255));
    return true;
}

bool parse_query_color_to_rgb(const std::string &query_key,
                              const std::string &query_value, rgb_color &out) {
    if (query_key == "hex") {
        return parse_hex_to_rgb(query_value, out);
    }
    if (query_key == "rgb") {
        return parse_rgb_to_rgb(query_value, out);
    }
    if (query_key == "hsl") {
        return parse_hsl_to_rgb(query_value, out);
    }
    if (query_key == "cmyk") {
        return parse_cmyk_to_rgb(query_value, out);
    }
    return false;
}

void rgb_to_hsl(rgb_color in, double &h, double &s, double &l) {
    const double r = static_cast<double>(in.r) / 255.0;
    const double g = static_cast<double>(in.g) / 255.0;
    const double b = static_cast<double>(in.b) / 255.0;

    const double max_v = std::max({r, g, b});
    const double min_v = std::min({r, g, b});
    const double delta = max_v - min_v;

    h = 0.0;
    s = 0.0;
    l = (max_v + min_v) / 2.0;

    if (delta > 0.0) {
        s = l > 0.5 ? delta / (2.0 - max_v - min_v)
                    : delta / (max_v + min_v);

        if (max_v == r) {
            h = (g - b) / delta + (g < b ? 6.0 : 0.0);
        } else if (max_v == g) {
            h = (b - r) / delta + 2.0;
        } else {
            h = (r - g) / delta + 4.0;
        }
        h *= 60.0;
    }

    s *= 100.0;
    l *= 100.0;
}

bool parse_color_list(const std::string &raw, color_model model,
                      std::vector<rgb_color> &out) {
    size_t start = 0;
    while (start <= raw.size()) {
        const size_t delimiter = raw.find(';', start);
        const std::string token =
            delimiter == std::string::npos ? raw.substr(start)
                                           : raw.substr(start, delimiter - start);

        const std::string trimmed = trim_copy(token);
        if (!trimmed.empty()) {
            rgb_color rgb{};
            bool ok = false;
            switch (model) {
            case color_model::hex:
                ok = parse_hex_to_rgb(trimmed, rgb);
                break;
            case color_model::rgb:
                ok = parse_rgb_to_rgb(trimmed, rgb);
                break;
            case color_model::hsl:
                ok = parse_hsl_to_rgb(trimmed, rgb);
                break;
            case color_model::cmyk:
                ok = parse_cmyk_to_rgb(trimmed, rgb);
                break;
            }

            if (!ok) {
                return false;
            }
            out.push_back(rgb);
        }

        if (delimiter == std::string::npos) {
            break;
        }
        start = delimiter + 1;
    }

    return !out.empty();
}

double relative_luminance(rgb_color value) {
    const double r = srgb_channel_to_linear(value.r);
    const double g = srgb_channel_to_linear(value.g);
    const double b = srgb_channel_to_linear(value.b);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double contrast_ratio(rgb_color a, rgb_color b) {
    const double l1 = relative_luminance(a);
    const double l2 = relative_luminance(b);
    const double lighter = std::max(l1, l2);
    const double darker = std::min(l1, l2);
    return (lighter + 0.05) / (darker + 0.05);
}

double contrast_ratio_on_black(rgb_color text_color) {
    return contrast_ratio(text_color, {0, 0, 0});
}

wcag_contrast_result evaluate_wcag_contrast(rgb_color text,
                                            rgb_color background) {
    wcag_contrast_result result;
    result.ratio = contrast_ratio(text, background);
    result.aa_normal = result.ratio >= 4.5;
    result.aa_large = result.ratio >= 3.0;
    result.aaa_normal = result.ratio >= 7.0;
    result.aaa_large = result.ratio >= 4.5;
    result.pass_count = static_cast<int>(result.aa_normal) +
                        static_cast<int>(result.aa_large) +
                        static_cast<int>(result.aaa_normal) +
                        static_cast<int>(result.aaa_large);
    result.rating_percent = result.pass_count * 25;
    return result;
}

bool is_web_safe_color(rgb_color value) {
    return is_web_safe_channel(value.r) && is_web_safe_channel(value.g) &&
           is_web_safe_channel(value.b);
}

rgb_color nearest_web_safe_color(rgb_color value) {
    return {
        static_cast<uint8_t>(nearest_web_safe_channel(value.r)),
        static_cast<uint8_t>(nearest_web_safe_channel(value.g)),
        static_cast<uint8_t>(nearest_web_safe_channel(value.b)),
    };
}

std::string rgb_to_hex(rgb_color value) {
    return "#" + to_hex_pair(value.r) + to_hex_pair(value.g) + to_hex_pair(value.b);
}

} // namespace palette::services
