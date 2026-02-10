#pragma once
#include "palette/services/palette_image.hpp"
#include <dpp/dpp.h>
#include <string>
#include <vector>

namespace palette::services {

enum class palette_control_mode { shades, tints };

struct palette_control_state {
    palette_control_mode mode = palette_control_mode::shades;
    std::vector<rgb_color> seed_colors;
    int amount = 2;
};

struct palette_render_result {
    bool ok = false;
    std::string error;
    std::string description;
    std::string image_data;
};

int clamp_palette_amount(int amount);

std::string create_palette_control_token(palette_control_mode mode,
                                         const std::vector<rgb_color> &seeds,
                                         int amount);
bool get_palette_control_state(const std::string &token,
                               palette_control_state &out);
bool adjust_palette_control_amount(const std::string &token, int delta,
                                   palette_control_state &out);

std::string build_palette_button_id(palette_control_mode mode, int delta,
                                    const std::string &token);
bool parse_palette_button_id(const std::string &custom_id,
                             palette_control_mode &mode, int &delta,
                             std::string &token);

dpp::component build_palette_controls_row(palette_control_mode mode, int amount,
                                          const std::string &token);
palette_render_result render_palette_with_controls(palette_control_mode mode,
                                                   const std::vector<rgb_color> &seeds,
                                                   int amount);

} // namespace palette::services
