#include "entur_departures.h"
#include <algorithm>
#include <cstdlib>
#include <string>

namespace esphome::entur_departures {

using tessera::Font;
namespace ui = tessera::ui;

namespace {

// Splits "20 Jernbanetorget" into the line's public code and where it goes. Home Assistant's entur_public_transport
// integration joins the two with one space; a public code never has one itself.
void split_route(const std::string &route, std::string &line, std::string &to) {
  const size_t sp = route.find(' ');
  if (sp == std::string::npos) {
    line = route;
    to.clear();
  } else {
    line = route.substr(0, sp);
    to = route.substr(sp + 1);
  }
}

// The leading number of a text such as "7 min" (the integration's own wording for the departure after next); -1 when
// there is none.
int leading_minutes(const std::string &text) {
  if (text.empty()) return -1;
  char *end = nullptr;
  const long n = strtol(text.c_str(), &end, 10);
  return end == text.c_str() ? -1 : static_cast<int>(n);
}

}  // namespace

// One departure as the sensor gives it: the line and where it goes, in how many minutes it was due the last time
// Home Assistant asked, how late it runs, and whether that estimate is live.
struct Departure {
  std::string line, to;
  int minutes = 0;
  int delay = 0;
  bool realtime = true;
  bool known = false;
};

// A departure adjusted for the time since the last update, ready to show: when the bus itself leaves, and, with a
// walk time set, when a person must leave to still catch it.
struct Shown {
  std::string line, to;
  int raw_minutes = 0;
  int go_minutes = 0;
  int delay = 0;
  bool realtime = true;
};

// The tile. Shows the next departure big, with its line and destination above it, and the one after it on a line
// below. It draws only what Home Assistant's entur_public_transport sensor already knows (its state, the minutes
// until the next departure, and a handful of attributes for it and the one after); the tile never asks Entur itself
// (docs/MAKING_A_PLUGIN.md, "Where the backend lives").
class NextTile : public tessera::Tile {
 public:
  explicit NextTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    walk_ = c.options["walk"] | 0;
    late_ = c.options["late"] | true;
    name_h_ = ui::line_height(Font::BODY);
    head_ = std::max(ui::line_height(Font::TITLE), ui::line_height(Font::BODY)) + ui::px(4);

    name_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_label_set_long_mode(name_, LV_LABEL_LONG_CLIP);
    lv_obj_set_size(name_, width_, name_h_);
    lv_obj_set_pos(name_, 0, 0);
    ui::set_text(name_, c.name);

    badge_ = ui::block(c.parent, theme::ACCENT);
    number_ = ui::label(badge_, Font::TITLE, theme::ON_ACCENT);
    lv_label_set_long_mode(number_, LV_LABEL_LONG_CLIP);
    lv_obj_center(number_);
    to_ = ui::label(c.parent, Font::BODY, theme::MUTED);

    big_ = ui::label(c.parent, Font::VALUE, theme::INK);
    lv_obj_set_width(big_, width_);
    lv_obj_set_style_text_align(big_, LV_TEXT_ALIGN_CENTER, 0);

    later_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_size(later_, width_, ui::line_height(Font::BODY));
    lv_obj_set_pos(later_, 0, height_ - ui::line_height(Font::BODY));
  }

  void on_state(JsonObjectConst data) override {
    has_entity_ = data["state"].is<const char *>();
    const std::string state = data["state"] | "";
    JsonObjectConst attrs = data["attributes"];

    current_ = Departure{};
    next_ = Departure{};
    unavailable_ = state.empty() || state == "unavailable" || state == "unknown";

    if (!unavailable_) {
      current_.minutes = static_cast<int>(strtol(state.c_str(), nullptr, 10));
      current_.delay = attrs["delay"] | 0;
      current_.realtime = attrs["real_time"] | true;
      current_.known = true;
      split_route(attrs["route"] | "", current_.line, current_.to);

      const int next_minutes = leading_minutes(attrs["next_due_in"] | "");
      if (next_minutes >= 0) {
        next_.minutes = next_minutes;
        next_.delay = attrs["next_delay"] | 0;
        next_.realtime = attrs["next_real_time"] | true;
        next_.known = true;
        split_route(attrs["next_route"] | "", next_.line, next_.to);
      }
    }
    // The countdown keeps running on the screen's own clock between two updates; a fresh update restarts it from
    // whatever the screen's clock last read (on_tick sets last_tick_ at least once a second).
    base_now_ = last_tick_;
    elapsed_ = 0;
  }

  void on_tick(uint32_t now) override {
    if (now && base_now_ && now > base_now_) elapsed_ = static_cast<int>((now - base_now_) / 60);
    last_tick_ = now;

    Shown d{}, n{};
    bool has_d = adjust(current_, d);
    bool has_n = adjust(next_, n);
    if (!has_d && has_n) {
      d = n;
      has_d = true;
      has_n = false;
    }

    if (!has_entity_ || unavailable_ || !has_d) {
      place_badge("", "");
      place_message(plugin_->text(!has_entity_ ? "choose_sensor" : "none"));
      ui::set_text(later_, "");
      return;
    }

    place_badge(d.line, with_delay(d));
    place_number(when_text(d.raw_minutes));
    if (walk_ > 0) {
      ui::set_text(later_, go_text(d.go_minutes));
    } else {
      ui::set_text(later_,
                    has_n ? tessera::fill(plugin_->text("then"), "when", when_text(n.raw_minutes) + "  " + n.to) : "");
    }
  }

  void on_theme() override {
    ui::set_color(name_, theme::MUTED);
    lv_obj_set_style_bg_color(badge_, ui::color(theme::ACCENT), 0);
    ui::set_color(number_, theme::ON_ACCENT);
    ui::set_color(to_, theme::MUTED);
    ui::set_color(big_, theme::INK);
    ui::set_color(later_, theme::MUTED);
  }

 private:
  // When the bus itself leaves (the time since the last update taken off) and, with a walk time set, when a person
  // must leave to still catch it; false once that is in the past, so the tile moves on to the one after.
  bool adjust(const Departure &dep, Shown &out) const {
    if (!dep.known) return false;
    const int raw = dep.minutes - elapsed_;
    const int go = raw - walk_;
    if (go < 0) return false;
    out.line = dep.line;
    out.to = dep.to;
    out.raw_minutes = raw;
    out.go_minutes = go;
    out.delay = dep.delay;
    out.realtime = dep.realtime;
    return true;
  }

  std::string with_delay(const Shown &d) const {
    if (!late_ || !d.realtime || d.delay < 1) return d.to;
    return d.to + "  " + tessera::format(plugin_->text("late"), static_cast<long>(d.delay));
  }

  std::string when_text(int minutes) const {
    if (minutes <= 0) return plugin_->text("now");
    return tessera::format(plugin_->text("minutes"), static_cast<long>(minutes));
  }

  // When to leave for the walk to the stop: "Leave in 12 min", then "Leave now" the minute it is due.
  std::string go_text(int minutes) const {
    if (minutes <= 0) return plugin_->text("go_now");
    return tessera::format(plugin_->text("go_in"), static_cast<long>(minutes));
  }

  void place_badge(const std::string &line, const std::string &to) {
    const bool shown = !line.empty();
    lv_obj_set_style_opa(badge_, shown ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    ui::set_text(number_, line);
    const int pad = ui::px(ui::large() ? 8 : 5);
    const int badge_w = shown ? std::min(width_ / 2, ui::text_width(line, Font::TITLE) + 2 * pad) : 0;
    lv_obj_set_size(badge_, std::max(1, badge_w), head_);
    lv_obj_set_pos(badge_, 0, name_h_);
    lv_obj_center(number_);
    const int x = shown ? badge_w + ui::px(6) : 0;
    ui::set_text(to_, to);
    lv_obj_set_size(to_, std::max(1, width_ - x), ui::line_height(Font::BODY));
    lv_obj_set_pos(to_, x, name_h_ + (head_ - ui::line_height(Font::BODY)) / 2);
  }

  // The time takes the largest face that fits the card, the way a clock does.
  Font fitting(const std::string &text) const {
    for (Font f : {Font::VALUE, Font::HEADLINE, Font::TITLE})
      if (ui::text_width(text, f) <= width_) return f;
    return Font::BODY_LARGE;
  }

  void place_number(const std::string &text) {
    const Font face = fitting(text);
    ui::set_font(big_, face);
    ui::set_text(big_, text);
    center_big(ui::line_height(face));
  }

  void place_message(const std::string &text) {
    ui::set_font(big_, Font::BODY_LARGE);
    ui::set_text(big_, text);
    center_big(ui::line_height(Font::BODY_LARGE));
  }

  void center_big(int h) {
    lv_obj_set_height(big_, h);
    const int top = name_h_ + head_, bottom = height_ - ui::line_height(Font::BODY);
    lv_obj_set_pos(big_, 0, top + std::max(0, (bottom - top - h) / 2));
  }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, head_ = 0, name_h_ = 0, walk_ = 0;
  bool late_ = true, has_entity_ = false, unavailable_ = false;
  uint32_t last_tick_ = 0, base_now_ = 0;
  int elapsed_ = 0;
  Departure current_, next_;
  lv_obj_t *name_{}, *badge_{}, *number_{}, *to_{}, *big_{}, *later_{};
};

void EnturDepartures::setup() {
  add_tile("next", [this]() { return new NextTile(this); });
}

}  // namespace esphome::entur_departures
