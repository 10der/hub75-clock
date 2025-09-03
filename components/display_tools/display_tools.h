// display_tools.h
#pragma once

// Мінімальні потрібні інклуди з ESPHome
#include "esphome.h"
#include "esphome/components/display/display.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/automation.h"
// #include <Adafruit_Protomatter.h>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <cmath>
#include <ctime>

namespace esphome {
namespace display_tools {

using esphome::display::Display;
using esphome::display::BaseFont;
using esphome::Color;
using esphome::display::TextAlign;

// ============================================================================
// Клас DisplayTools: все з мого порту сюди; глобальні -> поля; free-фн -> методи
// ============================================================================

struct RawWord {
  std::string text;
  std::string color;
};

enum class Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, COUNT };

enum class DrawCommandType { PIXEL, LINE, HLINE, VLINE, RECTANGLE, FILLED_RECTANGLE, TRIANGLE, FILLED_TRIANGLE, TEXT };

struct DrawObject {
  DrawCommandType type;
  int x1 = 0, y1 = 0;
  int x2 = 0, y2 = 0;
  int x3 = 0, y3 = 0;
  Color color = Color::WHITE;
  std::string text;          // для TEXT
  BaseFont *font = nullptr;  // для TEXT
  TextAlign align = TextAlign::BASELINE_LEFT;
};

class DisplayTools : public Component {
 public:
  // ---------- Публічні типи (були у твоєму .h) ----------
  struct ColoredWord {
    std::string text;
    Color color;
    BaseFont *font = nullptr;
  };

  struct ScrollingState {
    int16_t xpos = -1;
    int16_t repeat = 0;
    bool scrolling = false;
    std::string last_text;
    int frame_hold = 0;
  };

  struct App_Info {
    std::string name;
    std::string body;
    Color color = Color::WHITE;
    uint16_t duration = 2;
    std::string icon;
    Color icon_color = Color::WHITE;
    std::vector<ColoredWord> text_parts;
    std::vector<DrawObject> draw_objects;
    uint16_t index = 0;
    ScrollingState scroll;
  };

  struct AlertMessage {
    std::string text;
    Color color{255, 0, 0};
    std::string icon;  // unicode-рядок, напр. "\U000F05D6"
    Color icon_color{255, 0, 0};
    std::string sound{"14"};  // просто рядок-ідентифікатор
    uint16_t repeat{1};
    ScrollingState scroll;
  };

  Trigger<int> *get_on_play_trigger() { return this->on_play_trigger_; }

  // ---------- Життєвий цикл ESPHome ----------
  void setup() override;
  void dump_config() override;

  static Color hex_to_color(const std::string &hex);
  
  // --- API ---
  void render_main_screen(display::Display &it);
  void render_app_screen(display::Display &it);

  // --- setters ---
  void set_night_mode(bool state) { this->night_mode_state_ = state; }
  void set_clock_time(esphome::time::RealTimeClock *clock) { this->clock_time_ = clock; }
  // void set_dfplayer(esphome::dfplayer_pro::DFPlayerPro *player) { this->dfplayer_ = player; }

  void set_clock_font(display::BaseFont *f) { this->clock_font_ = f; }
  void set_app_font(display::BaseFont *f) { this->app_font_ = f; }
  void set_icon_font(display::BaseFont *f) { this->icon_font_ = f; }
  void set_extra_font(display::BaseFont *f) { this->extra_font_ = f; }

  // void set_scroll_speed(float speed) { this->scroll_speed_ = speed; }

  void set_temperature_outside(float temp) { this->temperature_outside_ = temp; }
  void set_temperature_inside(float temp) { this->temperature_inside_ = temp; }
  void set_weather_icon(const std::string &icon) { this->weather_icon_ = icon; }

  void set_temperature_progress(const std::vector<int> &progress) { this->temperature_progress_ = progress; }
  const std::vector<int> &get_temperature_progress() const { return this->temperature_progress_; }

  void set_top_left(bool v) { set_corner_state(Corner::TOP_LEFT, v); }
  void set_top_right(bool v) { set_corner_state(Corner::TOP_RIGHT, v); }
  void set_bottom_left(bool v) { set_corner_state(Corner::BOTTOM_LEFT, v); }
  void set_bottom_right(bool v) { set_corner_state(Corner::BOTTOM_RIGHT, v); }

  bool get_top_left() const { return get_corner_state(Corner::TOP_LEFT); }
  bool get_top_right() const { return get_corner_state(Corner::TOP_RIGHT); }
  bool get_bottom_left() const { return get_corner_state(Corner::BOTTOM_LEFT); }
  bool get_bottom_right() const { return get_corner_state(Corner::BOTTOM_RIGHT); }
  // float get_scroll_speed() const { return scroll_speed_; }

  bool get_night_mode() const { return night_mode_state_; }
  // ======================================================================
  //                        КЕРУВАННЯ ДОДАТКАМИ (apps)
  // ======================================================================
  void dump_app_info(const App_Info &info);
  void addApp(std::string name, std::string body = "", std::string color = "FFFFFF", uint16_t duration = 2,
              std::string icon = "", std::string icon_color = "FFFFFF", std::vector<ColoredWord> text_parts = {},
              std::vector<DrawObject> draw_objects = {});
  bool delApp(const std::string &name);
  void nextApp();
  App_Info *getCurrentApp();
  void reorderAppsByIndex();

  // ======================================================================
  //                      ЧЕРГА АЛЕРТІВ (було у тебе)
  // ======================================================================
  void addAlert(std::string text, std::string color, std::string icon, std::string icon_color, std::string sound,
                uint16_t repeat);
  bool hasAlert() const;
  bool getCurrentAlert(AlertMessage &out);
  void removeCurrentAlert();

  // ======================================================================
  //                           УТИЛІТИ (other)
  // ======================================================================

  std::vector<DisplayTools::ColoredWord> make_colored_words(const std::vector<std::string> &texts,
                                                            const std::vector<std::string> &colors, BaseFont *text_font,
                                                            BaseFont *icon_font);
  std::string get_app_loop();

  // ======================================================================
  //                           КІНЕЦЬ ПУБЛІЧНОГО API
  // ======================================================================

 private:
  static constexpr const char *TAG = "display_tools";
  static constexpr size_t npos = static_cast<size_t>(-1);
  Trigger<int> *on_play_trigger_{new Trigger<int>()};

  // state
  bool night_mode_state_{false};
  int tick_counter_{0};
  bool first_alert_play_{true};

  float temperature_outside_{NAN};
  float temperature_inside_{NAN};
  std::string weather_icon_;
  std::vector<int> temperature_progress_;

  // external deps
  esphome::time::RealTimeClock *clock_time_{nullptr};
  // esphome::dfplayer_pro::DFPlayerPro *dfplayer_{nullptr};

  // fonts
  display::BaseFont *clock_font_{nullptr};
  display::BaseFont *app_font_{nullptr};
  display::BaseFont *icon_font_{nullptr};
  display::BaseFont *extra_font_{nullptr};

  std::array<bool, static_cast<int>(Corner::COUNT)> corner_states_{};

  // colors
  static constexpr Color RED = Color(0xFF0000);
  static constexpr Color LIGHT_GRAY = Color(0xC0C0C0);
  static constexpr Color YELLOW = Color(0xFFFF00);
  static constexpr Color ORANGE = Color(0xFFA500);

  // ---------- Стан (раніше глобальні) ----------
  std::vector<App_Info> apps_;
  size_t current_app_index_{npos};
  std::queue<AlertMessage> alert_messages_queue_;

  // ---------- Приватні хелпери ----------
  App_Info *getAppByName_(const std::string &name);

  bool get_corner_state(Corner c) const { return corner_states_[static_cast<int>(c)]; }
  void set_corner_state(Corner c, bool value) { corner_states_[static_cast<int>(c)] = value; }

  // ======================================================================
  //                           УТИЛІТИ (раніше вільні функції)
  // ======================================================================

  static const char *get_icon_char(const std::string &icon_name);
  static std::string truncate_utf8_string(const std::string &str, size_t max_len);
  static std::string getLastSegment(const std::string &topic);
  static Color hsv_to_rgb(float h, float s, float v);
  static Color get_temp_color(float temp);
  static std::string to_upper(const std::string &input);
  static std::string cyr_upper(const std::string &str);

  // ======================================================================
  //                          МАЛЮВАЛКИ (як у тебе)
  // ======================================================================
  bool drawTodayDate(Display &it, BaseFont *font, int xpos, int ypos);
  bool drawScrollingTextWithIcon(Display &it, const std::string &text, const Color &textColor, const std::string &icon,
                                 const Color &iconColor, BaseFont *fontText, BaseFont *fontIcon, int repeat);
  bool drawScrollingTextWithIcon(Display &it, const std::vector<ColoredWord> &textParts, const std::string &icon,
                                 const Color &iconColor, BaseFont *fontIcon, int repeat);
  bool drawPagedTextWithIcon(Display &it, const std::string &text, const Color &textColor, const std::string &icon,
                             const Color &iconColor, BaseFont *fontText, BaseFont *fontIcon, int repeat);
  bool drawDrawObjects(Display &it, BaseFont *textFont, const std::vector<DrawObject> &objects);
  bool drawDrawObjectsWithIcon(Display &it, BaseFont *textFont, const std::vector<DrawObject> &objects,
                               const std::string &icon, BaseFont *iconFont, const Color &iconColor);
  void draw_colored_line(esphome::display::Display &it, std::vector<int> temp_forecast, bool night_mode_state);
  void draw_alert_corner(Display &it, Corner corner, const Color &color);

  void emit_on_play_sound(int no) {
    if (on_play_trigger_)
      on_play_trigger_->trigger(no);
  }
};

}  // namespace display_tools
}  // namespace esphome
