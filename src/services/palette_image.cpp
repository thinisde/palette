#include "palette/services/palette_image.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace palette::services {
namespace {
struct image {
    int width;
    int height;
    std::vector<uint8_t> pixels;
};

void fill_rect(image &img, int x, int y, int w, int h, rgb_color color) {
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(img.width, x + w);
    const int y1 = std::min(img.height, y + h);

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const size_t i = static_cast<size_t>((py * img.width + px) *
                                                 static_cast<int>(4));
            img.pixels[i + 0] = color.r;
            img.pixels[i + 1] = color.g;
            img.pixels[i + 2] = color.b;
            img.pixels[i + 3] = 255;
        }
    }
}

image make_image(int width, int height, rgb_color background) {
    image img{width, height,
              std::vector<uint8_t>(static_cast<size_t>(width * height * 4))};
    fill_rect(img, 0, 0, width, height, background);
    return img;
}

void append_u32_be(std::string &buffer, uint32_t value) {
    buffer.push_back(static_cast<char>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<char>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<char>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<char>(value & 0xFF));
}

const std::array<uint32_t, 256> &crc32_table() {
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> out{};
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                if (c & 1U) {
                    c = 0xEDB88320U ^ (c >> 1U);
                } else {
                    c >>= 1U;
                }
            }
            out[n] = c;
        }
        return out;
    }();
    return table;
}

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    const auto &table = crc32_table();
    for (size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc;
}

uint32_t adler32(std::string_view data) {
    constexpr uint32_t mod = 65521U;
    uint32_t s1 = 1U;
    uint32_t s2 = 0U;

    for (unsigned char byte : data) {
        s1 = (s1 + byte) % mod;
        s2 = (s2 + s1) % mod;
    }

    return (s2 << 16U) | s1;
}

std::string zlib_stored_compress(std::string_view raw) {
    std::string out;
    out.reserve(raw.size() + 6 + (raw.size() / 65535 + 1) * 5);
    out.push_back('\x78');
    out.push_back('\x01');

    size_t offset = 0;
    while (offset < raw.size()) {
        const size_t remaining = raw.size() - offset;
        const uint16_t len =
            static_cast<uint16_t>(std::min<size_t>(65535, remaining));
        const bool final_block = (offset + len) >= raw.size();

        out.push_back(static_cast<char>(final_block ? 0x01 : 0x00));
        const uint16_t nlen = static_cast<uint16_t>(~len);
        out.push_back(static_cast<char>(len & 0xFF));
        out.push_back(static_cast<char>((len >> 8) & 0xFF));
        out.push_back(static_cast<char>(nlen & 0xFF));
        out.push_back(static_cast<char>((nlen >> 8) & 0xFF));
        out.append(raw.data() + offset, len);
        offset += len;
    }

    append_u32_be(out, adler32(raw));
    return out;
}

void append_chunk(std::string &png, const char type[4],
                  const std::string &data) {
    append_u32_be(png, static_cast<uint32_t>(data.size()));
    png.append(type, 4);
    png.append(data);

    uint32_t crc = 0xFFFFFFFFU;
    crc = crc32_update(crc, reinterpret_cast<const uint8_t *>(type), 4);
    if (!data.empty()) {
        crc = crc32_update(crc, reinterpret_cast<const uint8_t *>(data.data()),
                           data.size());
    }
    append_u32_be(png, crc ^ 0xFFFFFFFFU);
}

std::string encode_png(const image &img) {
    std::string raw;
    raw.reserve(static_cast<size_t>(img.height) *
                (static_cast<size_t>(img.width) * 4 + 1));

    for (int y = 0; y < img.height; ++y) {
        raw.push_back('\x00');
        const size_t row_offset =
            static_cast<size_t>(y) * static_cast<size_t>(img.width) * 4;
        raw.append(reinterpret_cast<const char *>(img.pixels.data()) +
                       row_offset,
                   static_cast<size_t>(img.width) * 4);
    }

    std::string png;
    const unsigned char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    png.append(reinterpret_cast<const char *>(signature), 8);

    std::string ihdr;
    ihdr.reserve(13);
    append_u32_be(ihdr, static_cast<uint32_t>(img.width));
    append_u32_be(ihdr, static_cast<uint32_t>(img.height));
    ihdr.push_back('\x08');
    ihdr.push_back('\x06');
    ihdr.push_back('\x00');
    ihdr.push_back('\x00');
    ihdr.push_back('\x00');

    append_chunk(png, "IHDR", ihdr);
    append_chunk(png, "IDAT", zlib_stored_compress(raw));
    append_chunk(png, "IEND", std::string());
    return png;
}

rgb_color number_color(rgb_color swatch) {
    const double luminance =
        0.299 * swatch.r + 0.587 * swatch.g + 0.114 * swatch.b;
    return luminance < 145.0 ? rgb_color{255, 255, 255} : rgb_color{10, 10, 10};
}

const std::array<std::array<const char *, 5>, 10> &digit_patterns() {
    static const std::array<std::array<const char *, 5>, 10> patterns = {{
        {"111", "101", "101", "101", "111"},
        {"010", "110", "010", "010", "111"},
        {"111", "001", "111", "100", "111"},
        {"111", "001", "111", "001", "111"},
        {"101", "101", "111", "001", "001"},
        {"111", "100", "111", "001", "111"},
        {"111", "100", "111", "101", "111"},
        {"111", "001", "010", "010", "010"},
        {"111", "101", "111", "101", "111"},
        {"111", "101", "111", "001", "111"},
    }};
    return patterns;
}

void draw_digit(image &img, int digit, int x, int y, int scale,
                rgb_color color) {
    if (digit < 0 || digit > 9) {
        return;
    }

    const auto &rows = digit_patterns()[digit];
    for (size_t row = 0; row < rows.size(); ++row) {
        for (int col = 0; col < 3; ++col) {
            if (rows[row][col] != '1') {
                continue;
            }
            fill_rect(img, x + col * scale, y + static_cast<int>(row) * scale,
                      scale, scale, color);
        }
    }
}

void draw_number(image &img, int number, int center_x, int center_y, int scale,
                 rgb_color color) {
    const std::string text = std::to_string(std::max(0, number));
    const int digit_width = 3 * scale;
    const int spacing = std::max(1, scale / 2);
    const int total_width = static_cast<int>(text.size()) * digit_width +
                            static_cast<int>(text.size() - 1) * spacing;
    const int start_x = center_x - total_width / 2;
    const int y = center_y - (5 * scale) / 2;

    int x = start_x;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            draw_digit(img, c - '0', x, y, scale, color);
        }
        x += digit_width + spacing;
    }
}

const std::array<const char *, 7> &glyph_pattern(char c) {
    static const std::array<const char *, 7> blank = {
        "00000", "00000", "00000", "00000", "00000", "00000", "00000"};
    static const std::array<const char *, 7> dot = {
        "00000", "00000", "00000", "00000", "00000", "00110", "00110"};
    static const std::array<const char *, 7> dash = {
        "00000", "00000", "00000", "11111", "00000", "00000", "00000"};

    static const std::array<const char *, 7> a = {
        "01110", "10001", "10001", "11111", "10001", "10001", "10001"};
    static const std::array<const char *, 7> b = {
        "11110", "10001", "10001", "11110", "10001", "10001", "11110"};
    static const std::array<const char *, 7> c_pattern = {
        "01111", "10000", "10000", "10000", "10000", "10000", "01111"};
    static const std::array<const char *, 7> d = {
        "11110", "10001", "10001", "10001", "10001", "10001", "11110"};
    static const std::array<const char *, 7> e = {
        "11111", "10000", "10000", "11110", "10000", "10000", "11111"};
    static const std::array<const char *, 7> f = {
        "11111", "10000", "10000", "11110", "10000", "10000", "10000"};
    static const std::array<const char *, 7> g = {
        "01111", "10000", "10000", "10011", "10001", "10001", "01111"};
    static const std::array<const char *, 7> h = {
        "10001", "10001", "10001", "11111", "10001", "10001", "10001"};
    static const std::array<const char *, 7> i = {
        "11111", "00100", "00100", "00100", "00100", "00100", "11111"};
    static const std::array<const char *, 7> j = {
        "00111", "00010", "00010", "00010", "10010", "10010", "01100"};
    static const std::array<const char *, 7> k = {
        "10001", "10010", "10100", "11000", "10100", "10010", "10001"};
    static const std::array<const char *, 7> l = {
        "10000", "10000", "10000", "10000", "10000", "10000", "11111"};
    static const std::array<const char *, 7> m = {
        "10001", "11011", "10101", "10101", "10001", "10001", "10001"};
    static const std::array<const char *, 7> n = {
        "10001", "11001", "10101", "10011", "10001", "10001", "10001"};
    static const std::array<const char *, 7> o = {
        "01110", "10001", "10001", "10001", "10001", "10001", "01110"};
    static const std::array<const char *, 7> p = {
        "11110", "10001", "10001", "11110", "10000", "10000", "10000"};
    static const std::array<const char *, 7> q = {
        "01110", "10001", "10001", "10001", "10101", "10010", "01101"};
    static const std::array<const char *, 7> r = {
        "11110", "10001", "10001", "11110", "10100", "10010", "10001"};
    static const std::array<const char *, 7> s = {
        "01111", "10000", "10000", "01110", "00001", "00001", "11110"};
    static const std::array<const char *, 7> t = {
        "11111", "00100", "00100", "00100", "00100", "00100", "00100"};
    static const std::array<const char *, 7> u = {
        "10001", "10001", "10001", "10001", "10001", "10001", "01110"};
    static const std::array<const char *, 7> v = {
        "10001", "10001", "10001", "10001", "10001", "01010", "00100"};
    static const std::array<const char *, 7> w = {
        "10001", "10001", "10001", "10101", "10101", "10101", "01010"};
    static const std::array<const char *, 7> x = {
        "10001", "10001", "01010", "00100", "01010", "10001", "10001"};
    static const std::array<const char *, 7> y = {
        "10001", "10001", "01010", "00100", "00100", "00100", "00100"};
    static const std::array<const char *, 7> z = {
        "11111", "00001", "00010", "00100", "01000", "10000", "11111"};

    static const std::array<const char *, 7> zero = {
        "01110", "10001", "10011", "10101", "11001", "10001", "01110"};
    static const std::array<const char *, 7> one = {
        "00100", "01100", "00100", "00100", "00100", "00100", "01110"};
    static const std::array<const char *, 7> two = {
        "01110", "10001", "00001", "00010", "00100", "01000", "11111"};
    static const std::array<const char *, 7> three = {
        "11110", "00001", "00001", "01110", "00001", "00001", "11110"};
    static const std::array<const char *, 7> four = {
        "00010", "00110", "01010", "10010", "11111", "00010", "00010"};
    static const std::array<const char *, 7> five = {
        "11111", "10000", "10000", "11110", "00001", "00001", "11110"};
    static const std::array<const char *, 7> six = {
        "01110", "10000", "10000", "11110", "10001", "10001", "01110"};
    static const std::array<const char *, 7> seven = {
        "11111", "00001", "00010", "00100", "01000", "01000", "01000"};
    static const std::array<const char *, 7> eight = {
        "01110", "10001", "10001", "01110", "10001", "10001", "01110"};
    static const std::array<const char *, 7> nine = {
        "01110", "10001", "10001", "01111", "00001", "00001", "01110"};

    switch (c) {
    case '.':
        return dot;
    case '-':
        return dash;
    case 'A':
        return a;
    case 'B':
        return b;
    case 'C':
        return c_pattern;
    case 'D':
        return d;
    case 'E':
        return e;
    case 'F':
        return f;
    case 'G':
        return g;
    case 'H':
        return h;
    case 'I':
        return i;
    case 'J':
        return j;
    case 'K':
        return k;
    case 'L':
        return l;
    case 'M':
        return m;
    case 'N':
        return n;
    case 'O':
        return o;
    case 'P':
        return p;
    case 'Q':
        return q;
    case 'R':
        return r;
    case 'S':
        return s;
    case 'T':
        return t;
    case 'U':
        return u;
    case 'V':
        return v;
    case 'W':
        return w;
    case 'X':
        return x;
    case 'Y':
        return y;
    case 'Z':
        return z;
    case '0':
        return zero;
    case '1':
        return one;
    case '2':
        return two;
    case '3':
        return three;
    case '4':
        return four;
    case '5':
        return five;
    case '6':
        return six;
    case '7':
        return seven;
    case '8':
        return eight;
    case '9':
        return nine;
    default:
        return blank;
    }
}

void draw_glyph(image &img, char c, int x, int y, int scale, rgb_color color) {
    const char normalized =
        static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    const auto &rows = glyph_pattern(normalized);
    for (size_t row = 0; row < rows.size(); ++row) {
        for (int col = 0; col < 5; ++col) {
            if (rows[row][col] != '1') {
                continue;
            }
            fill_rect(img, x + col * scale, y + static_cast<int>(row) * scale,
                      scale, scale, color);
        }
    }
}

int text_char_spacing(int scale) { return std::max(1, scale / 2); }

int text_line_width(std::string_view line, int scale) {
    if (line.empty()) {
        return 0;
    }
    const int spacing = text_char_spacing(scale);
    return static_cast<int>(line.size()) * (5 * scale) +
           static_cast<int>(line.size() - 1) * spacing;
}

void draw_text_line(image &img, std::string_view line, int x, int y, int scale,
                    rgb_color color) {
    const int spacing = text_char_spacing(scale);
    int cursor = x;
    for (const char c : line) {
        draw_glyph(img, c, cursor, y, scale, color);
        cursor += 5 * scale + spacing;
    }
}

std::vector<rgb_color> make_shades_to_black(rgb_color seed, int shade_count) {
    std::vector<rgb_color> shades;
    shades.reserve(shade_count);

    for (int i = 0; i < shade_count; ++i) {
        const double t =
            shade_count > 1 ? static_cast<double>(i) / (shade_count - 1) : 1.0;
        const double keep = 1.0 - t;
        shades.push_back(
            {static_cast<uint8_t>(std::clamp(
                 static_cast<int>(std::round(seed.r * keep)), 0, 255)),
             static_cast<uint8_t>(std::clamp(
                 static_cast<int>(std::round(seed.g * keep)), 0, 255)),
             static_cast<uint8_t>(std::clamp(
                 static_cast<int>(std::round(seed.b * keep)), 0, 255))});
    }

    if (!shades.empty()) {
        shades.back() = {0, 0, 0};
    }
    return shades;
}

std::vector<rgb_color> make_tints_to_white_impl(rgb_color seed, int tint_count) {
    std::vector<rgb_color> tints;
    tints.reserve(tint_count);

    for (int i = 0; i < tint_count; ++i) {
        const double t =
            tint_count > 1 ? static_cast<double>(i) / (tint_count - 1) : 1.0;
        tints.push_back(
            {static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(
                                     seed.r + (255.0 - seed.r) * t)),
                                             0, 255)),
             static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(
                                     seed.g + (255.0 - seed.g) * t)),
                                             0, 255)),
             static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(
                                     seed.b + (255.0 - seed.b) * t)),
                                             0, 255))});
    }

    if (!tints.empty()) {
        tints.front() = seed;
        tints.back() = {255, 255, 255};
    }

    return tints;
}

std::string render_step_palette_image(
    const std::vector<std::vector<rgb_color>> &palette) {
    if (palette.empty()) {
        return std::string();
    }

    const int rows = static_cast<int>(palette.size());
    const int cols = static_cast<int>(palette.front().size());
    if (cols <= 0) {
        return std::string();
    }

    constexpr int canvas_width = 800;
    constexpr int canvas_height = 600;
    constexpr int margin_x = 40;
    constexpr int margin_y = 40;
    constexpr int gap = 8;

    const int available_width = canvas_width - (margin_x * 2);
    const int available_height = canvas_height - (margin_y * 2);
    const int swatch_width =
        (available_width - (cols - 1) * gap) / std::max(cols, 1);
    const int swatch_height =
        (available_height - (rows - 1) * gap) / std::max(rows, 1);

    const int grid_width = cols * swatch_width + (cols - 1) * gap;
    const int grid_height = rows * swatch_height + (rows - 1) * gap;
    const int start_x = (canvas_width - grid_width) / 2;
    const int start_y = (canvas_height - grid_height) / 2;

    image img = make_image(canvas_width, canvas_height, {255, 255, 255});
    const int digit_scale =
        std::clamp(std::min(swatch_width / 6, swatch_height / 8), 3, 12);
    const int digit_width = 3 * digit_scale;
    const int digit_height = 5 * digit_scale;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const int x = start_x + col * (swatch_width + gap);
            const int y = start_y + row * (swatch_height + gap);
            const rgb_color swatch = palette[row][col];
            fill_rect(img, x, y, swatch_width, swatch_height, swatch);

            const rgb_color label_color = number_color(swatch);
            const int label_x = x + (swatch_width - digit_width) / 2;
            const int label_y = y + (swatch_height - digit_height) / 2;
            draw_digit(img, col + 1, label_x, label_y, digit_scale,
                       label_color);
        }
    }

    return encode_png(img);
}

} // namespace

image_result generate_color_palette(const std::vector<rgb_color> &colors,
                                    int amount,
                                    bool colors_are_steps) {
    image_result result;
    if (colors.empty()) {
        return result;
    }

    const int resolved_count = std::clamp(amount, 2, 8);
    if (colors_are_steps) {
        const size_t rows = (colors.size() + static_cast<size_t>(resolved_count) - 1) /
                            static_cast<size_t>(resolved_count);
        result.palette.reserve(rows);

        for (size_t row = 0; row < rows; ++row) {
            std::vector<rgb_color> line;
            line.reserve(static_cast<size_t>(resolved_count));
            for (int col = 0; col < resolved_count; ++col) {
                const size_t index =
                    row * static_cast<size_t>(resolved_count) + static_cast<size_t>(col);
                if (index < colors.size()) {
                    line.push_back(colors[index]);
                } else if (!line.empty()) {
                    line.push_back(line.back());
                } else {
                    line.push_back({0, 0, 0});
                }
            }
            result.palette.push_back(std::move(line));
        }
    } else {
        result.palette.reserve(colors.size());
        for (const rgb_color seed : colors) {
            result.palette.push_back(make_shades_to_black(seed, resolved_count));
        }
    }

    result.image_data = render_step_palette_image(result.palette);
    return result;
}

std::vector<rgb_color> make_tints_to_white(rgb_color seed, int amount) {
    return make_tints_to_white_impl(seed, std::clamp(amount, 2, 8));
}

std::string generate_palette_image(const std::vector<rgb_color> &colors) {
    return generate_palette_image(colors, false);
}

std::string generate_palette_image(const std::vector<rgb_color> &colors,
                                   bool include_numbers) {
    if (colors.empty()) {
        return std::string();
    }

    constexpr int canvas_width = 800;
    constexpr int canvas_height = 600;
    constexpr int margin_x = 40;
    constexpr int margin_y = 40;
    constexpr int gap = 8;

    const int total = static_cast<int>(colors.size());
    const int cols = std::min(5, total);
    const int rows = (total + cols - 1) / cols;

    const int available_width = canvas_width - (margin_x * 2);
    const int available_height = canvas_height - (margin_y * 2);
    const int swatch_width =
        (available_width - (cols - 1) * gap) / std::max(cols, 1);
    const int swatch_height =
        (available_height - (rows - 1) * gap) / std::max(rows, 1);

    image img = make_image(canvas_width, canvas_height, {255, 255, 255});
    const int digit_scale =
        std::clamp(std::min(swatch_width / 6, swatch_height / 8), 3, 12);

    for (int i = 0; i < total; ++i) {
        const int row = i / cols;
        const int col = i % cols;
        const int x = margin_x + col * (swatch_width + gap);
        const int y = margin_y + row * (swatch_height + gap);
        fill_rect(img, x, y, swatch_width, swatch_height, colors[i]);

        if (include_numbers) {
            const rgb_color label_color = number_color(colors[i]);
            draw_number(img, i + 1, x + swatch_width / 2, y + swatch_height / 2,
                        digit_scale, label_color);
        }
    }

    return encode_png(img);
}

std::string
generate_text_on_background_image(const std::vector<std::string> &lines,
                                  rgb_color text_color,
                                  rgb_color background_color) {
    if (lines.empty()) {
        return std::string();
    }

    constexpr int canvas_width = 800;
    constexpr int canvas_height = 600;
    constexpr int margin_x = 40;
    constexpr int margin_y = 40;

    image img = make_image(canvas_width, canvas_height, background_color);

    int scale = 2;
    for (int candidate = 20; candidate >= 2; --candidate) {
        int max_width = 0;
        for (const std::string &line : lines) {
            max_width = std::max(max_width, text_line_width(line, candidate));
        }
        const int line_height = 7 * candidate;
        const int line_gap = std::max(candidate, 4);
        const int block_height =
            static_cast<int>(lines.size()) * line_height +
            static_cast<int>(lines.size() - 1) * line_gap;

        if (max_width <= (canvas_width - 2 * margin_x) &&
            block_height <= (canvas_height - 2 * margin_y)) {
            scale = candidate;
            break;
        }
    }

    const int line_height = 7 * scale;
    const int line_gap = std::max(scale, 4);
    const int block_height = static_cast<int>(lines.size()) * line_height +
                             static_cast<int>(lines.size() - 1) * line_gap;
    int y = (canvas_height - block_height) / 2;

    for (const std::string &line : lines) {
        const int width = text_line_width(line, scale);
        const int x = (canvas_width - width) / 2;
        draw_text_line(img, line, x, y, scale, text_color);
        y += line_height + line_gap;
    }

    return encode_png(img);
}

std::string generate_text_on_black_image(const std::vector<std::string> &lines,
                                         rgb_color text_color) {
    return generate_text_on_background_image(lines, text_color, {0, 0, 0});
}

std::string generate_text_on_white_image(const std::vector<std::string> &lines,
                                         rgb_color text_color) {
    return generate_text_on_background_image(lines, text_color,
                                             {255, 255, 255});
}

} // namespace palette::services
