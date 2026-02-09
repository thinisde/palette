#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace palette::services {

struct rgb_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct image_result {
    std::string image_data;
    std::vector<std::vector<rgb_color>> palette;
};

image_result generate_color_palette(const std::vector<rgb_color> &colors,
                                    int amount,
                                    bool colors_are_steps = false);
std::vector<rgb_color> make_tints_to_white(rgb_color seed, int amount);
std::string generate_palette_image(const std::vector<rgb_color> &colors);
std::string generate_palette_image(const std::vector<rgb_color> &colors,
                                   bool include_numbers);
std::string
generate_text_on_background_image(const std::vector<std::string> &lines,
                                  rgb_color text_color,
                                  rgb_color background_color);
std::string generate_text_on_black_image(const std::vector<std::string> &lines,
                                         rgb_color text_color);
std::string generate_text_on_white_image(const std::vector<std::string> &lines,
                                         rgb_color text_color);

} // namespace palette::services
