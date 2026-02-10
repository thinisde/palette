#pragma once
#include "palette/services/palette_image.hpp"
#include <dpp/dpp.h>
#include <string>
#include <string_view>
#include <vector>

namespace palette::services {

enum class color_model { hex, rgb, hsl, cmyk };

struct single_color_input_result {
    bool ok = false;
    std::string error;
    std::string query_key;
    std::string query_value;
};

struct multi_color_input_result {
    bool ok = false;
    std::string error;
    std::vector<rgb_color> colors;
};

struct wcag_contrast_result {
    bool aa_normal = false;
    bool aa_large = false;
    bool aaa_normal = false;
    bool aaa_large = false;
    int pass_count = 0;
    int rating_percent = 0;
    double ratio = 1.0;
};

bool read_optional_string(const dpp::command_value &value, std::string &out);
single_color_input_result
parse_single_color_input(const dpp::slashcommand_t &event);
multi_color_input_result
parse_multi_color_input(const dpp::slashcommand_t &event, size_t max_colors);

std::string trim_copy(std::string_view value);

bool parse_hex_to_rgb(const std::string &value, rgb_color &out);
bool parse_rgb_to_rgb(const std::string &value, rgb_color &out);
bool parse_hsl_to_rgb(const std::string &value, rgb_color &out);
bool parse_cmyk_to_rgb(const std::string &value, rgb_color &out);
bool parse_query_color_to_rgb(const std::string &query_key,
                              const std::string &query_value, rgb_color &out);
bool hsl_to_rgb(double h, double s, double l, rgb_color &out);
void rgb_to_hsl(rgb_color in, double &h, double &s, double &l);

bool parse_color_list(const std::string &raw, color_model model,
                      std::vector<rgb_color> &out);

double relative_luminance(rgb_color value);
double contrast_ratio(rgb_color a, rgb_color b);
double contrast_ratio_on_black(rgb_color text_color);
wcag_contrast_result evaluate_wcag_contrast(rgb_color text,
                                            rgb_color background);
bool is_web_safe_color(rgb_color value);
rgb_color nearest_web_safe_color(rgb_color value);
rgb_color mix_colors(const std::vector<rgb_color> &colors);
std::string rgb_to_hex(rgb_color value);

} // namespace palette::services
