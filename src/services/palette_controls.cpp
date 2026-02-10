#include "palette/services/palette_controls.hpp"
#include "palette/services/color_utils.hpp"
#include <algorithm>
#include <atomic>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace palette::services {
namespace {
constexpr int kMinAmount = 2;
constexpr int kMaxAmount = 8;
constexpr size_t kMaxStates = 1024;

std::mutex state_mutex;
std::unordered_map<std::string, palette_control_state> state_by_token;
std::vector<std::string> insertion_order;
std::atomic<uint64_t> token_counter{0};

std::string next_token() {
    const uint64_t value = ++token_counter;
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(12) << value;
    return stream.str();
}

void prune_old_states_locked() {
    if (state_by_token.size() <= kMaxStates) {
        return;
    }

    size_t index = 0;
    while (state_by_token.size() > kMaxStates &&
           index < insertion_order.size()) {
        state_by_token.erase(insertion_order[index]);
        ++index;
    }

    if (index > 0) {
        insertion_order.erase(insertion_order.begin(),
                              insertion_order.begin() +
                                  static_cast<long>(index));
    }
}

std::string
format_palette_details(const std::vector<std::vector<rgb_color>> &palette,
                       const char *final_label) {
    std::string out;
    for (size_t row = 0; row < palette.size(); ++row) {
        const auto &steps = palette[row];
        if (steps.empty()) {
            continue;
        }

        if (palette.size() > 1) {
            out += "**Color " + std::to_string(row + 1) + "**\n";
        }

        for (size_t i = 0; i < steps.size(); ++i) {
            out += std::to_string(i + 1) + ". " + rgb_to_hex(steps[i]);
            if (i == 0) {
                out += " (original)";
            } else if (i + 1 == steps.size()) {
                out += std::string(" (") + final_label + ")";
            }
            out += "\n";
        }

        if (row + 1 != palette.size()) {
            out += "\n";
        }
    }
    return out;
}
} // namespace

int clamp_palette_amount(int amount) {
    return std::clamp(amount, kMinAmount, kMaxAmount);
}

std::string create_palette_control_token(palette_control_mode mode,
                                         const std::vector<rgb_color> &seeds,
                                         int amount) {
    if (seeds.empty()) {
        return std::string();
    }

    palette_control_state state;
    state.mode = mode;
    state.seed_colors = seeds;
    state.amount = clamp_palette_amount(amount);

    const std::string token = next_token();
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        state_by_token[token] = state;
        insertion_order.push_back(token);
        prune_old_states_locked();
    }
    return token;
}

bool get_palette_control_state(const std::string &token,
                               palette_control_state &out) {
    std::lock_guard<std::mutex> lock(state_mutex);
    auto it = state_by_token.find(token);
    if (it == state_by_token.end()) {
        return false;
    }
    out = it->second;
    return true;
}

bool adjust_palette_control_amount(const std::string &token, int delta,
                                   palette_control_state &out) {
    std::lock_guard<std::mutex> lock(state_mutex);
    auto it = state_by_token.find(token);
    if (it == state_by_token.end()) {
        return false;
    }

    it->second.amount = clamp_palette_amount(it->second.amount + delta);
    out = it->second;
    return true;
}

std::string build_palette_button_id(palette_control_mode mode, int delta,
                                    const std::string &token) {
    if (token.empty()) {
        return std::string();
    }

    const std::string prefix =
        mode == palette_control_mode::shades ? "shades_" : "tints_";
    const std::string action = delta > 0 ? "next" : "back";
    return prefix + action + ":" + token;
}

bool parse_palette_button_id(const std::string &custom_id,
                             palette_control_mode &mode, int &delta,
                             std::string &token) {
    const auto parse = [&](const char *prefix,
                           palette_control_mode candidate_mode,
                           int candidate_delta) -> bool {
        const std::string full_prefix = std::string(prefix) + ":";
        if (custom_id.rfind(full_prefix, 0) != 0) {
            return false;
        }
        token = custom_id.substr(full_prefix.size());
        if (token.empty()) {
            return false;
        }
        mode = candidate_mode;
        delta = candidate_delta;
        return true;
    };

    if (parse("shades_next", palette_control_mode::shades, +1) ||
        parse("shades_back", palette_control_mode::shades, -1) ||
        parse("tints_next", palette_control_mode::tints, +1) ||
        parse("tints_back", palette_control_mode::tints, -1)) {
        return true;
    }

    return false;
}

dpp::component build_palette_controls_row(palette_control_mode mode, int amount,
                                          const std::string &token) {
    const int clamped_amount = clamp_palette_amount(amount);
    dpp::component row;
    row.set_type(dpp::cot_action_row);

    if (clamped_amount > kMinAmount) {
        row.add_component(
            dpp::component()
                .set_label("-1")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_primary)
                .set_id(build_palette_button_id(mode, -1, token)));
    }

    if (clamped_amount < kMaxAmount) {
        row.add_component(
            dpp::component()
                .set_label("+1")
                .set_type(dpp::cot_button)
                .set_style(dpp::cos_primary)
                .set_id(build_palette_button_id(mode, +1, token)));
    }

    return row;
}

palette_render_result
render_palette_with_controls(palette_control_mode mode,
                             const std::vector<rgb_color> &seeds, int amount) {
    palette_render_result result;
    if (seeds.empty()) {
        result.error = "No source colors to render.";
        return result;
    }

    const int clamped_amount = clamp_palette_amount(amount);

    if (mode == palette_control_mode::shades) {
        const image_result image =
            generate_color_palette(seeds, clamped_amount);
        if (image.image_data.empty()) {
            result.error = "Failed to generate color palette image.";
            return result;
        }

        std::string description =
            format_palette_details(image.palette, "black");
        if (description.empty()) {
            description = "Generated color palette.";
        } else {
            description =
                "A shade is a concept of darkening a color.\n\n" + description;
        }

        result.description = std::move(description);
        result.image_data = image.image_data;
        result.ok = true;
        return result;
    }

    std::vector<rgb_color> tint_steps;
    tint_steps.reserve(seeds.size() * static_cast<size_t>(clamped_amount));
    for (const rgb_color seed : seeds) {
        std::vector<rgb_color> row = make_tints_to_white(seed, clamped_amount);
        tint_steps.insert(tint_steps.end(), row.begin(), row.end());
    }

    const image_result image =
        generate_color_palette(tint_steps, clamped_amount, true);
    if (image.image_data.empty()) {
        result.error = "Failed to generate tint palette image.";
        return result;
    }

    std::string description = format_palette_details(image.palette, "white");
    if (description.empty()) {
        description = "Generated tint palette.";
    } else {
        description =
            "A tint is a concept of lightening a color.\n\n" + description;
    }

    result.description = std::move(description);
    result.image_data = image.image_data;
    result.ok = true;
    return result;
}

} // namespace palette::services
