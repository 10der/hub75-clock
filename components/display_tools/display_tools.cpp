// display_tools.cpp
#include "display_tools.h"
#include <string>
#include <sstream>
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

inline std::string trim(const std::string &s) {
  auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
  auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
  return (start < end) ? std::string(start, end) : "";
}

std::string strip_emojis(const std::string &input) {
  std::string result;
  for (size_t i = 0; i < input.size();) {
    unsigned char c = input[i];

    // ASCII
    if (c < 0x80) {
      result += c;
      i++;
    }
    // 2-байтові символи UTF-8 (U+0080..U+07FF)
    else if ((c & 0xE0) == 0xC0 && i + 1 < input.size()) {
      result.append(input, i, 2);
      i += 2;
    }
    // 3-байтові символи UTF-8 (U+0800..U+FFFF) — залишаємо
    else if ((c & 0xF0) == 0xE0 && i + 2 < input.size()) {
      result.append(input, i, 3);
      i += 3;
    }
    // 4-байтові (U+10000 і вище, тобто емодзі) — пропускаємо
    else if ((c & 0xF8) == 0xF0 && i + 3 < input.size()) {
      i += 4;  // ❌ emoji, скіпаємо
    } else {
      // некоректний байт
      i++;
    }
  }
  return result;
}

// ---------- Життєвий цикл ESPHome ----------
void DisplayTools::setup() {
  ESP_LOGI(TAG, "DisplayTools setup complete");
  addApp("__date__");
}

void DisplayTools::dump_config() {
  ESP_LOGCONFIG(TAG, "DisplayTools: apps=%u, alerts in queue=%u", (unsigned) apps_.size(),
                (unsigned) alert_messages_queue_.size());
}

// ======================================================================
//                           УТИЛІТИ (раніше вільні функції)
// ======================================================================
const char *DisplayTools::get_icon_char(const std::string &icon_name) {
  static const std::map<std::string, const char *> icon_map = {{"mdi:weather-night", "\U000F0594"},
                                                               {"mdi:weather-cloudy", "\U000F0590"},
                                                               {"mdi:weather-fog", "\U000F0591"},
                                                               {"mdi:weather-hail", "\U000F0592"},
                                                               {"mdi:weather-lightning", "\U000F0593"},
                                                               {"mdi:weather-lightning-rainy", "\U000F067E"},
                                                               {"mdi:weather-partly-cloudy", "\U000F0595"},
                                                               {"mdi:weather-pouring", "\U000F0596"},
                                                               {"mdi:weather-rainy", "\U000F0597"},
                                                               {"mdi:weather-snowy", "\U000F0598"},
                                                               {"mdi:weather-snowy-rainy", "\U000F067F"},
                                                               {"mdi:weather-sunny", "\U000F0599"},
                                                               {"mdi:weather-windy", "\U000F059D"},
                                                               {"mdi:weather-windy-variant", "\U000F059E"},
                                                               {"mdi:weather-night-partly-cloudy", "\U000F0F31"},
                                                               {"mdi:alert-circle-outline", "\U000F05D6"},
                                                               {"mdi:music", "\U000F075A"},
                                                               {"mdi:weather-sunset", "\U000F059A"},
                                                               {"mdi:weather-sunset-down", "\U000F059B"},
                                                               {"mdi:weather-sunset-up", "\U000F059C"},
                                                               {"mdi:thermometer-lines", "\U000F0510"},
                                                               {"mdi:water-percent", "\U000F058E"},
                                                               {"mdi:quality-high", "\U000F0435"},
                                                               {"mdi:quality-low", "\U000F0A0C"},
                                                               {"mdi:quality-medium", "\U000F0A0D"},
                                                               {"mdi:bell-alert", "\U000F0D59"},
                                                               {"mdi:cake-variant", "\U000F00EB"},
                                                               {"mdi:application-cog", "\U000F0675"},
                                                               {"mdi:moon-first-quarter", "\U000F0F61"},
                                                               {"mdi:moon-full", "\U000F0F62"},
                                                               {"mdi:moon-last-quarter", "\U000F0F63"},
                                                               {"mdi:moon-new", "\U000F0F64"},
                                                               {"mdi:moon-waning-crescent", "\U000F0F65"},
                                                               {"mdi:moon-waning-gibbous", "\U000F0F66"},
                                                               {"mdi:moon-waxing-crescent", "\U000F0F67"},
                                                               {"mdi:moon-waxing-gibbous", "\U000F0F68"},
                                                               {"mdi:home-thermometer", "\U000F0F54"},
                                                               {"mdi:sun-thermometer-outline", "\U000F18D7"},
                                                               {"mdi:washing-machine", "\U000F072A"},
                                                               {"mdi:calendar-alert", "\U000F0A31"},
                                                               {"mdi:sleep", "\U000F04B2"},
                                                               {"mdi:bed-clock", "\U000F1B94"}};
  auto it = icon_map.find(icon_name);
  if (it != icon_map.end())
    return it->second;
  return "";
}

std::string DisplayTools::getLastSegment(const std::string &topic) {
  if (topic.empty())
    return topic;
  size_t pos = topic.find_last_of('/');
  return (pos == std::string::npos) ? topic : topic.substr(pos + 1);
}

Color DisplayTools::hex_to_color(const std::string &hex) {
  auto hex_char_to_int = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
      return 10 + (c - 'A');
    return 0;
  };
  if (hex.size() == 6) {
    int r = hex_char_to_int(hex[0]) * 16 + hex_char_to_int(hex[1]);
    int g = hex_char_to_int(hex[2]) * 16 + hex_char_to_int(hex[3]);
    int b = hex_char_to_int(hex[4]) * 16 + hex_char_to_int(hex[5]);
    return Color(r, g, b);
  }
  return Color::WHITE;
}

Color DisplayTools::hsv_to_rgb(float h, float s, float v) {
  h = fmodf(h, 360.0f);
  s = std::min(1.0f, std::max(0.0f, s));
  v = std::min(1.0f, std::max(0.0f, v));
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = v - c;
  float rf = 0, gf = 0, bf = 0;
  if (0 <= h && h < 60) {
    rf = c;
    gf = x;
    bf = 0;
  } else if (60 <= h && h < 120) {
    rf = x;
    gf = c;
    bf = 0;
  } else if (120 <= h && h < 180) {
    rf = 0;
    gf = c;
    bf = x;
  } else if (180 <= h && h < 240) {
    rf = 0;
    gf = x;
    bf = c;
  } else if (240 <= h && h < 300) {
    rf = x;
    gf = 0;
    bf = c;
  } else {
    rf = c;
    gf = 0;
    bf = x;
  }
  return Color((rf + m) * 255, (gf + m) * 255, (bf + m) * 255);
}

Color DisplayTools::get_temp_color(float temp) {
  if (temp <= -10)
    return Color(0, 0, 255);
  if (temp <= 0)
    return Color(0, 128, 255);
  if (temp <= 10)
    return Color(0, 200, 200);
  if (temp <= 20)
    return Color(0, 255, 0);
  if (temp <= 30)
    return Color(255, 200, 0);
  return Color(255, 0, 0);
}

std::string DisplayTools::to_upper(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for (unsigned char c : input)
    out.push_back(std::toupper(c));
  return out;
}

std::string DisplayTools::truncate_utf8_string(const std::string &str, size_t max_len) {
  size_t len = 0;
  size_t i = 0;
  while (i < str.size()) {
    unsigned char c = str[i];
    size_t char_len = 1;
    if ((c & 0x80) == 0x00)
      char_len = 1;  // 0xxxxxxx (ASCII)
    else if ((c & 0xE0) == 0xC0)
      char_len = 2;  // 110xxxxx
    else if ((c & 0xF0) == 0xE0)
      char_len = 3;  // 1110xxxx
    else if ((c & 0xF8) == 0xF0)
      char_len = 4;  // 11110xxx
    else
      break;  // неправильний символ

    if (len + 1 > max_len)
      break;  // ліміт по символах
    i += char_len;
    len++;
  }
  return str.substr(0, i);
}

std::string DisplayTools::cyr_upper(const std::string &str) {
  static const std::unordered_map<std::string, std::string> upper_map = {
      {"а", "А"}, {"б", "Б"}, {"в", "В"}, {"г", "Г"}, {"ґ", "Ґ"}, {"д", "Д"}, {"е", "Е"}, {"є", "Є"}, {"ж", "Ж"},
      {"з", "З"}, {"и", "И"}, {"і", "І"}, {"ї", "Ї"}, {"й", "Й"}, {"к", "К"}, {"л", "Л"}, {"м", "М"}, {"н", "Н"},
      {"о", "О"}, {"п", "П"}, {"р", "Р"}, {"с", "С"}, {"т", "Т"}, {"у", "У"}, {"ф", "Ф"}, {"х", "Х"}, {"ц", "Ц"},
      {"ч", "Ч"}, {"ш", "Ш"}, {"щ", "Щ"}, {"ь", "Ь"}, {"ю", "Ю"}, {"я", "Я"}};

  std::string result;
  for (size_t i = 0; i < str.size();) {
    unsigned char c = str[i];
    size_t char_len = 1;
    if ((c & 0x80) == 0x00) {
      // ASCII
      result += static_cast<char>(std::toupper(c));
    } else if ((c & 0xE0) == 0xC0)
      char_len = 2;
    else if ((c & 0xF0) == 0xE0)
      char_len = 3;
    else if ((c & 0xF8) == 0xF0)
      char_len = 4;

    std::string ch = str.substr(i, char_len);
    auto it = upper_map.find(ch);
    if (it != upper_map.end()) {
      result += it->second;
    } else if (char_len > 1) {
      result += ch;  // залишаємо як є (цифри, смайли, інше)
    }
    i += char_len;
  }
  return result;
}

// ======================================================================
//                        КЕРУВАННЯ ДОДАТКАМИ (apps)
// ======================================================================
void DisplayTools::addApp(std::string name, std::string body, std::string color, uint16_t duration, std::string icon,
                          std::string icon_color, std::vector<ColoredWord> text_parts,
                          std::vector<DrawObject> draw_objects) {
  App_Info *found = getAppByName_(name);

  if (!body.empty()) {
    body = cyr_upper(body);
  }

  if (found != nullptr) {
    found->body = body;
    found->color = hex_to_color(color);
    found->duration = (duration == 0) ? 2 : duration;
    found->icon = get_icon_char(icon);
    found->icon_color = hex_to_color(icon_color);
    found->text_parts = std::move(text_parts);
    found->draw_objects = std::move(draw_objects);
    ESP_LOGI(TAG, "Updated app: %s", name.c_str());
    return;
  }

  App_Info app;
  app.name = name;
  app.body = body;
  app.color = hex_to_color(color);
  app.duration = (duration == 0) ? 2 : duration;
  app.icon = get_icon_char(icon);
  app.icon_color = hex_to_color(icon_color);
  app.text_parts = std::move(text_parts);
  app.draw_objects = std::move(draw_objects);
  app.index = apps_.empty() ? 0 : (apps_.back().index + 1);

  apps_.push_back(std::move(app));
  if (current_app_index_ == npos)
    current_app_index_ = 0;
  ESP_LOGI(TAG, "Added app: %s", name.c_str());
}

bool DisplayTools::delApp(const std::string &name) {
  for (auto it = apps_.begin(); it != apps_.end(); ++it) {
    if (it->name == name) {
      size_t deleted = std::distance(apps_.begin(), it);
      apps_.erase(it);
      if (apps_.empty()) {
        current_app_index_ = npos;
      } else if (current_app_index_ >= apps_.size()) {
        current_app_index_ = apps_.size() - 1;
      } else if (deleted <= current_app_index_ && current_app_index_ > 0) {
        current_app_index_--;
      }
      ESP_LOGI(TAG, "Deleted app: %s", name.c_str());
      return true;
    }
  }
  return false;
}

void DisplayTools::nextApp() {
  if (apps_.empty()) {
    current_app_index_ = npos;
    return;
  }
  if (current_app_index_ == npos)
    current_app_index_ = 0;
  else
    current_app_index_ = (current_app_index_ + 1) % apps_.size();
}

DisplayTools::App_Info *DisplayTools::getCurrentApp() {
  if (apps_.empty() || current_app_index_ == npos)
    return nullptr;
  return &apps_[current_app_index_];
}

void DisplayTools::reorderAppsByIndex() {
  std::sort(apps_.begin(), apps_.end(), [](const App_Info &a, const App_Info &b) { return a.index < b.index; });
}

std::string DisplayTools::get_app_loop() {
  std::string result = "[";
  bool first = true;
  for (const auto &app : this->apps_) {
    if (!first) {
      result += ",";
    } else {
      first = false;
    }
    result += "\"" + app.name + "\"";
  }
  result += "]";
  return result;
}

// ======================================================================
//                      ЧЕРГА АЛЕРТІВ (було у тебе)
// ======================================================================
void DisplayTools::addAlert(std::string text, std::string color, std::string icon, std::string icon_color,
                            std::string sound, uint16_t repeat) {
  AlertMessage alert;

  text = strip_emojis(text);
  text = trim(text);

  alert.text = cyr_upper(text);

  if (icon.empty()) {
    icon = "mdi:alert-circle-outline";
  }

  if (color.empty()) {
    color = "FF0000";
  }

  if (icon_color.empty()) {
    icon_color = "FF0000";
  }
  if (sound.empty()) {
    sound = "14";
  }

  alert.color = hex_to_color(color);
  alert.icon = get_icon_char(icon);
  alert.icon_color = hex_to_color(icon_color);
  alert.sound = sound;
  alert.repeat = repeat;

  alert_messages_queue_.push(alert);
  ESP_LOGI(TAG, "Added alert to queue: %s", text.c_str());
}

bool DisplayTools::hasAlert() const { return !alert_messages_queue_.empty(); }

bool DisplayTools::getCurrentAlert(AlertMessage &out) {
  if (alert_messages_queue_.empty())
    return false;
  out = alert_messages_queue_.front();
  return true;
}

void DisplayTools::removeCurrentAlert() {
  if (!alert_messages_queue_.empty())
    alert_messages_queue_.pop();
  first_alert_play_ = true;
}

// ======================================================================
//                          МАЛЮВАЛКИ (як у тебе)
// ======================================================================
bool DisplayTools::drawTodayDate(Display &it, BaseFont *font, int xpos, int ypos) {
  // як в оригіналі
  int repeat = 2;
  static int frame_hold = 0;
  const int hold_frames = 250 * repeat;  // ~2 сек при ~8ms кадрі

  // час/дата
  time_t now = ::time(nullptr);
  struct tm *local_time = ::localtime(&now);
  int day_of_month = local_time ? local_time->tm_mday : 1;

  // tm_wday: 0=нд, 1=пн, ..., 6=сб → робимо пн=0, нд=6
  int day_of_week = local_time ? local_time->tm_wday : 0;
  int adjusted_day_of_week = (day_of_week == 0) ? 6 : day_of_week - 1;

  // фон для дати (24x24)
  it.filled_rectangle(xpos, ypos - 16, 8 * 3, 8 * 3, Color(240, 240, 240));

  // заголовок (верхній рядок, червона смужка 24x6)
  it.filled_rectangle(xpos, ypos - 16, 8 * 3, 2 * 3, Color(240, 0, 0));

  // число місяця
  it.printf(xpos + 5, ypos + 7, font, Color::BLACK, TextAlign::BASELINE_LEFT, "%02d", day_of_month);

  // рисочки днів тижня
  int dash_x_start = xpos + 32;
  int dash_y = ypos + 6;
  int dash_width = 8;
  int dash_spacing = 4;
  int dash_height = 2;

  for (uint8_t i = 0; i < 7; i++) {
    Color dash_color = (i == adjusted_day_of_week) ? Color(240, 240, 240) : Color(100, 100, 100);
    it.filled_rectangle(dash_x_start + i * (dash_width + dash_spacing), dash_y, dash_width, dash_height, dash_color);
  }

  // назва місяця укр (точно як у тебе, з пробілами)
  const std::vector<std::string> months_uk = {"",        "Січень",   "Лютий",  "Березень", "Квітень",
                                              "Травень", "Червень",  "Липень", "Серпень",  "Вересень",
                                              "Жовтень", "Листопад", "Грудень"};
  std::string month = months_uk[local_time ? (local_time->tm_mon + 1) : 0];

  const int left_boundary = 32;

  int text_width, text_height;
  int dummy_y, dummy_x, dummy_h;
  month = cyr_upper(month);
  it.get_text_bounds(0, ypos, month.c_str(), font, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &text_width,
                     &text_height);

  const int available_width = it.get_width() - left_boundary - 12;

  // it.print(xpos + 32, ypos + 4, font, Color::WHITE, TextAlign::BASELINE_LEFT, month.c_str());
  const int center_x = left_boundary + (available_width - text_width) / 2;
  it.print(center_x, ypos + 3, font, Color::WHITE, TextAlign::BASELINE_LEFT, month.c_str());

  // затримка на кадрах
  frame_hold++;
  if (frame_hold >= hold_frames) {
    frame_hold = 0;
    return true;
  }
  return false;
}

// ORIGINAL
/*
bool DisplayTools::drawScrollingTextWithIcon(Display &it, const std::string &text, const Color &textColor,
                                             const std::string &icon, const Color &iconColor, BaseFont *fontText,
                                             BaseFont *fontIcon, int repeat) {
  int ypos = 56;

  int left_boundary = 0;
  int icon_width, dummy_y, dummy_x, dummy_h;
  if (!icon.empty()) {
    it.print(0, ypos, fontIcon, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
    it.get_text_bounds(0, ypos, icon.c_str(), fontIcon, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &icon_width,
                       &dummy_h);
    left_boundary = icon_width + 1;
  }

  static int16_t xpos = it.get_width();
  static int16_t xrepeat = 0;
  static std::string last_text;
  static bool scrolling = false;

  // Визначаємо ширину тексту
  int text_width, text_height;
  it.get_text_bounds(0, ypos, text.c_str(), fontText, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &text_width,
                     &text_height);

  int available_width = it.get_width() - left_boundary;

  // Скидаємо, якщо текст змінився
  if (text != last_text) {
    last_text = text;
    xrepeat = 0;
    xpos = it.get_width();
    scrolling = text_width > available_width;
  }

  static int frame_hold = 0;
  const int hold_frames = 250 * repeat;  // ~2 секунди при 8ms

  // Якщо текст вміщується — просто малюємо по центру
  if (!scrolling) {
    int center_x = left_boundary + (available_width - text_width) / 2;
    it.print(center_x, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());

    frame_hold++;
    if (frame_hold >= hold_frames) {
      frame_hold = 0;
      last_text = "";
      return true;
    }
    return false;
  }

  // Скролінг
  it.start_clipping(left_boundary, ypos - text_height, it.get_width(), ypos + text_height);
  auto clip = it.get_clipping();
  int clipping_left = clip.x;
  int clipping_right = clip.x + clip.w;

  int reset_threshold = clipping_left - text_width;

  if (xpos < reset_threshold) {
    xpos = clipping_right;
    xrepeat++;
    if (xrepeat >= repeat) {
      last_text = "";
      xrepeat = 0;
      scrolling = false;
      it.end_clipping();
      return true;
    }
  }

  it.print(xpos, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());
  it.end_clipping();
  xpos--;

  return false;
}
*/

/*
// ORIGINAL + FIXED SCROLL
bool DisplayTools::drawScrollingTextWithIcon(Display &it, const std::string &text, const Color &textColor,
                                             const std::string &icon, const Color &iconColor, BaseFont *fontText,
                                             BaseFont *fontIcon, int repeat) {
    // ---- Статичні змінні для стану
    static std::string last_text;
    static bool scrolling = false;
    static float xpos = 0.0f;          // float для субпіксельного руху
    static int16_t xrepeat = 0;
    static int text_width = 0, text_height = 0;

    static uint32_t last_step_us = 0;   // мікросекунди для точності
    static uint32_t hold_start_ms = 0;
    static float scroll_accum = 0.0f;   // накопичення дробових пікселів

    const int ypos = 56;

    // ---- Іконка зліва
    int left_boundary = 0;
    if (!icon.empty()) {
        it.print(0, ypos, fontIcon, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
        int icon_w, dummy_h, dummy_x, dummy_y;
        it.get_text_bounds(0, ypos, icon.c_str(), fontIcon, TextAlign::BASELINE_LEFT,
                           &dummy_x, &dummy_y, &icon_w, &dummy_h);
        left_boundary = icon_w + 1;
    }
    const int available_width = it.get_width() - left_boundary;

    // ---- Якщо текст змінився — скинути все
    if (text != last_text) {
        last_text = text;
        int dummy_x, dummy_y;
        it.get_text_bounds(0, ypos, text.c_str(), fontText, TextAlign::BASELINE_LEFT,
                           &dummy_x, &dummy_y, &text_width, &text_height);
        scrolling = (text_width > available_width);
        xrepeat   = 0;
        xpos      = static_cast<float>(it.get_width());
        last_step_us  = micros();
        hold_start_ms = millis();
        scroll_accum  = 0.0f;
    }

    // ---- Якщо текст влазить — просто показати і потримати N мс
    if (!scrolling) {
        const uint32_t hold_ms = 2000u * repeat;
        const int center_x = left_boundary + (available_width - text_width) / 2;
        it.print(center_x, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());
        if ((millis() - hold_start_ms) >= hold_ms) {
            last_text.clear();
            return true;
        }
        return false;
    }

    // ---- Рух: стабільний px/sec з накопиченням дробової частини
    uint32_t now_us = micros();
    uint32_t dt_us  = now_us - last_step_us;
    scroll_accum += scroll_speed_ * (static_cast<float>(dt_us) / 1000000.0f);

    int move = static_cast<int>(scroll_accum);   // скільки цілих пікселів рухати
    if (move != 0) {
        xpos -= move;
        scroll_accum -= move;                      // залишаємо дробову частину
        last_step_us = now_us;
    }

    // ---- Кліпінг
    it.start_clipping(left_boundary, ypos - text_height, it.get_width(), ypos + text_height);
    auto clip = it.get_clipping();
    const int clipping_left  = clip.x;
    const int clipping_right = clip.x + clip.w;
    const int reset_threshold = clipping_left - text_width;

    // ---- Коли текст вийшов за межі
    if (xpos < reset_threshold) {
        xpos = static_cast<float>(clipping_right);
        xrepeat++;
        if (xrepeat >= repeat) {
            last_text.clear();
            xrepeat = 0;
            scrolling = false;
            it.end_clipping();
            return true;
        }
    }

    // ---- Малюємо
    it.print(static_cast<int>(xpos), ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());
    it.end_clipping();
    return false;
}
*/

// FIXED SCROLL: таймерний піксельний крок (без тремтіння)
bool DisplayTools::drawScrollingTextWithIcon(Display &it, const std::string &text, const Color &textColor,
                                             const std::string &icon, const Color &iconColor, BaseFont *fontText,
                                             BaseFont *fontIcon, int repeat) {
  // ---- Статичні змінні для стану
  static std::string last_text;
  static bool scrolling = false;
  static int xpos = 0;
  static int16_t xrepeat = 0;
  static int text_width = 0, text_height = 0;

  static uint32_t hold_start_ms = 0;
  static uint32_t last_px_step = 0;  // таймер для руху
  const int ypos = 56;

  // ---- Іконка зліва
  int left_boundary = 0;
  if (!icon.empty()) {
    it.print(0, ypos, fontIcon, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
    int icon_w, dummy_h, dummy_x, dummy_y;
    it.get_text_bounds(0, ypos, icon.c_str(), fontIcon, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &icon_w,
                       &dummy_h);
    left_boundary = icon_w + 1;
  }
  const int available_width = it.get_width() - left_boundary;

  // ---- Якщо текст змінився — скинути все
  if (text != last_text) {
    last_text = text;
    int dummy_x, dummy_y;
    it.get_text_bounds(0, ypos, text.c_str(), fontText, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &text_width,
                       &text_height);
    scrolling = (text_width > available_width);
    xrepeat = 0;
    xpos = it.get_width();  // старт справа за екраном
    hold_start_ms = millis();
    last_px_step = millis();
  }

  // ---- Якщо текст влазить — просто показати і потримати N мс
  if (!scrolling) {
    const uint32_t hold_ms = 2000u * repeat;
    const int center_x = left_boundary + (available_width - text_width) / 2;
    it.print(center_x, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());
    if ((millis() - hold_start_ms) >= hold_ms) {
      last_text.clear();
      return true;
    }
    return false;
  }

  const float scroll_speed = 100;  // пікселів за секунду
  // int step = std::max(1, static_cast<int>(scroll_speed / 60.0f));

  // ---- Рух: стабільний крок по таймеру
  const uint32_t px_interval = 1000 / scroll_speed;  // мс на 1 піксель
  if (millis() - last_px_step >= px_interval) {
    xpos -= 1;
    last_px_step = millis();
  }

  // ---- Кліпінг
  it.start_clipping(left_boundary, ypos - text_height, it.get_width(), ypos + text_height);
  auto clip = it.get_clipping();
  const int clipping_left = clip.x;
  const int clipping_right = clip.x + clip.w;
  const int reset_threshold = clipping_left - text_width;

  // ---- Коли текст вийшов за межі
  if (xpos < reset_threshold) {
    xpos = clipping_right;
    xrepeat++;
    if (xrepeat >= repeat) {
      last_text.clear();
      xrepeat = 0;
      scrolling = false;
      it.end_clipping();
      return true;
    }
  }

  // ---- Малюємо
  it.print(xpos, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, text.c_str());
  it.end_clipping();
  return false;
}

bool DisplayTools::drawPagedTextWithIcon(Display &it, const std::string &text, const Color &textColor,
                                         const std::string &icon, const Color &iconColor, BaseFont *fontText,
                                         BaseFont *fontIcon, int repeat) {
  static std::vector<std::string> pages;
  static size_t current_page = 0;
  static std::string last_text;
  static uint32_t hold_start_ms = 0;
  const int ypos = 56;

  // ---- Іконка
  int left_boundary = 0;
  if (!icon.empty()) {
    it.print(0, ypos, fontIcon, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
    int icon_w, dummy_h, dummy_x, dummy_y;
    it.get_text_bounds(0, ypos, icon.c_str(), fontIcon, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &icon_w,
                       &dummy_h);
    left_boundary = icon_w + 1;
  }
  const int available_width = it.get_width() - left_boundary;

  // ---- Якщо текст новий — розбити на сторінки
  if (text != last_text) {
    last_text = text;
    pages.clear();
    current_page = 0;

    std::istringstream iss(text);
    std::string word, line;
    int line_width = 0;

    while (iss >> word) {
      std::string test_line = line.empty() ? word : line + " " + word;
      int dummy_x, dummy_y, test_w, test_h;
      it.get_text_bounds(0, ypos, test_line.c_str(), fontText, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &test_w,
                         &test_h);

      if (test_w <= available_width) {
        line = test_line;
        line_width = test_w;
      } else {
        if (!line.empty())
          pages.push_back(line);
        line = word;
      }
    }
    if (!line.empty())
      pages.push_back(line);

    hold_start_ms = millis();
  }

  // ---- Показати поточну сторінку
  if (current_page < pages.size()) {
    const std::string &page_text = pages[current_page];
    int dummy_x, dummy_y, text_w, text_h;
    it.get_text_bounds(0, ypos, page_text.c_str(), fontText, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &text_w,
                       &text_h);

    const int center_x = left_boundary + (available_width - text_w) / 2;
    it.print(center_x, ypos, fontText, textColor, TextAlign::BASELINE_LEFT, page_text.c_str());

    const uint32_t hold_ms = 2000u * repeat;
    if ((millis() - hold_start_ms) >= hold_ms) {
      current_page++;
      hold_start_ms = millis();
    }
    return false;
  }

  // ---- Кінець
  last_text.clear();
  pages.clear();
  current_page = 0;
  return true;
}

bool DisplayTools::drawScrollingTextWithIcon(Display &it, const std::vector<ColoredWord> &textParts,
                                             const std::string &icon, const Color &iconColor, BaseFont *fontIcon,
                                             int repeat) {
  int ypos = 56;

  int left_boundary = 0;
  int icon_width = 0;
  int dummy_y, dummy_x, dummy_h;

  if (!icon.empty()) {
    it.print(0, ypos, fontIcon, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
    it.get_text_bounds(0, ypos, icon.c_str(), fontIcon, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &icon_width,
                       &dummy_h);
    left_boundary = icon_width + 1;
  }

  // Статичні змінні для збереження стану скролінгу між викликами
  static int16_t xpos = 0;
  static int16_t xrepeat = 0;
  static std::string last_text_combined;
  static bool scrolling = false;
  static unsigned long last_update = 0;
  const unsigned long SCROLL_SPEED = 10;  // Чим менше значення, тим швидше скролінг

  // Вимірюємо загальну ширину всіх частин тексту
  int total_text_width = 0;
  std::string current_text_combined;
  int max_font_height = 0;

  for (const auto &part : textParts) {
    if (part.text.empty() || part.font == nullptr) {
      continue;
    }
    int part_width, part_height;
    it.get_text_bounds(0, ypos, part.text.c_str(), part.font, esphome::display::TextAlign::BASELINE_LEFT, &dummy_x,
                       &dummy_y, &part_width, &part_height);
    // Додаємо відступ між частинами тексту
    part_width += 2;

    total_text_width += part_width;
    current_text_combined += part.text;
    if (part_height > max_font_height) {
      max_font_height = part_height;
    }
  }

  // Визначаємо доступну ширину для тексту, враховуючи іконку
  int available_width = it.get_width() - left_boundary;

  // Скидаємо стан, якщо текст змінився
  if (current_text_combined != last_text_combined) {
    last_text_combined = current_text_combined;
    xrepeat = 0;
    xpos = available_width;
    scrolling = total_text_width > available_width;
    last_update = millis();
    // ESP_LOGD(TAG, "Текст змінився. Новий стан: 'scrolling': %s", scrolling ? "true" : "false");
  }

  // Якщо текст поміщається без скролінгу
  if (!scrolling) {
    int current_x = left_boundary + (available_width - total_text_width) / 2;
    for (const auto &part : textParts) {
      if (part.text.empty() || part.font == nullptr) {
        continue;
      }
      it.print(current_x, ypos, part.font, part.color, esphome::display::TextAlign::BASELINE_LEFT, part.text.c_str());
      int part_width;
      it.get_text_bounds(0, ypos, part.text.c_str(), part.font, esphome::display::TextAlign::BASELINE_LEFT, &dummy_x,
                         &dummy_y, &part_width, &dummy_h);
      // Додаємо відступ між частинами тексту
      part_width += 2;

      current_x += part_width;
    }

    static unsigned long hold_time_start = 0;
    const unsigned long hold_duration = 2000;

    if (hold_time_start == 0) {
      hold_time_start = millis();
    }

    if (millis() - hold_time_start >= hold_duration) {
      hold_time_start = 0;
      xrepeat++;
      if (xrepeat >= repeat) {
        last_text_combined = "";
        xrepeat = 0;
        scrolling = false;
        return true;
      }
    }
    return false;
  }

  // Скролінг
  it.start_clipping(left_boundary, ypos - max_font_height, it.get_width(), ypos + max_font_height);

  if (millis() - last_update >= SCROLL_SPEED) {
    xpos--;
    last_update = millis();
  }

  int current_x = left_boundary + xpos;

  // Малюємо кожну частину тексту
  for (const auto &part : textParts) {
    if (part.text.empty() || part.font == nullptr) {
      continue;
    }
    it.print(current_x, ypos, part.font, part.color, esphome::display::TextAlign::BASELINE_LEFT, part.text.c_str());
    int part_width, part_height;
    it.get_text_bounds(0, ypos, part.text.c_str(), part.font, esphome::display::TextAlign::BASELINE_LEFT, &dummy_x,
                       &dummy_y, &part_width, &part_height);
    // Додаємо відступ між частинами тексту
    part_width += 2;
    current_x += part_width;
  }

  it.end_clipping();

  // Перевіряємо, чи потрібно скинути скролінг
  int reset_threshold = -(total_text_width);
  if (xpos < reset_threshold) {
    xpos = available_width;
    xrepeat++;
    if (xrepeat >= repeat) {
      last_text_combined = "";
      xrepeat = 0;
      scrolling = false;
      return true;
    }
  }

  return false;
}

bool DisplayTools::drawDrawObjects(Display &it, BaseFont *textFont, const std::vector<DrawObject> &objects) {
  for (const auto &cmd : objects) {
    switch (cmd.type) {
      case DrawCommandType::PIXEL:
        it.draw_pixel_at(cmd.x1, cmd.y1, cmd.color);
        break;
      case DrawCommandType::LINE:
        it.line(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.color);
        break;
      case DrawCommandType::HLINE:
        it.horizontal_line(cmd.x1, cmd.y1, cmd.x2, cmd.color);
        break;
      case DrawCommandType::VLINE:
        it.vertical_line(cmd.x1, cmd.y1, cmd.y2, cmd.color);
        break;
      case DrawCommandType::RECTANGLE:
        it.rectangle(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.color);
        break;
      case DrawCommandType::FILLED_RECTANGLE:
        it.filled_rectangle(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.color);
        break;
      case DrawCommandType::TRIANGLE:
        it.triangle(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.x3, cmd.y3, cmd.color);
        break;
      case DrawCommandType::FILLED_TRIANGLE:
        it.filled_triangle(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.x3, cmd.y3, cmd.color);
        break;
      case DrawCommandType::TEXT: {
        BaseFont *f = cmd.font ? cmd.font : textFont;
        it.print(cmd.x1, cmd.y1, f, cmd.color, cmd.align, cmd.text.c_str());
        break;
      }
    }
  }
  return true;
}

bool DisplayTools::drawDrawObjectsWithIcon(Display &it, BaseFont *textFont, const std::vector<DrawObject> &objects,
                                           const std::string &icon, BaseFont *iconFont, const Color &iconColor) {
  int left_boundary = 0;
  int ypos = 56;
  int icon_width = 0, dummy_y = 0, dummy_x = 0, dummy_h = 0;
  if (!icon.empty()) {
    it.print(0, ypos, iconFont, iconColor, TextAlign::BASELINE_LEFT, icon.c_str());
    it.get_text_bounds(0, ypos, icon.c_str(), iconFont, TextAlign::BASELINE_LEFT, &dummy_x, &dummy_y, &icon_width,
                       &dummy_h);
    left_boundary = icon_width + 1;
  }
  std::vector<DrawObject> shifted = objects;
  for (auto &o : shifted) {
    o.x1 += left_boundary;
    o.x2 += left_boundary;
    o.x3 += left_boundary;
  }
  return drawDrawObjects(it, textFont, shifted);
}

// ---------- Приватні хелпери ----------
DisplayTools::App_Info *DisplayTools::getAppByName_(const std::string &name) {
  for (auto &app : apps_)
    if (app.name == name)
      return &app;
  return nullptr;
}

void DisplayTools::draw_colored_line(esphome::display::Display &it, std::vector<int> temp_forecast,
                                     bool night_mode_state) {
  const int screen_width = it.get_width();
  const int screen_height = it.get_height();
  const int line_y = screen_height / 2;

  // Якщо масив порожній, малюємо просту сіру лінію
  if (night_mode_state || temp_forecast.empty()) {
    it.line(0, line_y, screen_width, line_y, night_mode_state ? RED : LIGHT_GRAY);
    return;
  }

  // Обчислюємо розмір "кроку" для проходження по масиву
  const int total_temps = temp_forecast.size();

  // Малюємо лінію, використовуючи апроксимацію на всю довжину екрана
  for (size_t i = 0; i < total_temps; ++i) {
    // Обчислюємо початкову і кінцеву x-координати
    // Використовуємо кастинг до double, щоб уникнути помилок округлення
    int start_x = static_cast<int>(static_cast<double>(screen_width) * i / total_temps);
    int end_x = static_cast<int>(static_cast<double>(screen_width) * (i + 1) / total_temps);

    // Якщо це останній сегмент, переконаємося, що він досягає кінця екрана
    if (i == total_temps - 1) {
      end_x = screen_width;
    }

    // Отримуємо колір для поточної температури
    esphome::Color temp_color = get_temp_color(temp_forecast[i]);

    // Малюємо лінію для поточного сегмента
    it.line(start_x, line_y, end_x, line_y, temp_color);
  }
}

void DisplayTools::draw_alert_corner(Display &it, Corner corner, const Color &color) {
  switch (corner) {
    case Corner::TOP_LEFT:
      it.line(0, 0, 4, 0, color);
      it.line(0, 0, 0, 4, color);
      break;
    case Corner::TOP_RIGHT:
      it.line(it.get_width() - 5, 0, it.get_width() - 1, 0, color);
      it.line(it.get_width() - 1, 0, it.get_width() - 1, 4, color);
      break;
    case Corner::BOTTOM_LEFT:
      it.line(0, it.get_height() - 1, 4, it.get_height() - 1, color);
      it.line(0, it.get_height() - 5, 0, it.get_height() - 1, color);
      break;
    case Corner::BOTTOM_RIGHT:
      it.line(it.get_width() - 5, it.get_height() - 1, it.get_width() - 1, it.get_height() - 1, color);
      it.line(it.get_width() - 1, it.get_height() - 5, it.get_width() - 1, it.get_height() - 1, color);
      break;
  }
}

void DisplayTools::render_main_screen(display::Display &it) {
  const int blink_interval = 125;
  this->tick_counter_++;
  bool tick = (this->tick_counter_ / blink_interval) % 2 == 0;

  // --- Clock ---
  if (this->clock_time_ != nullptr && this->clock_font_ != nullptr) {
    char str[17];
    time_t currTime = this->clock_time_->now().timestamp;
    if (tick)
      strftime(str, sizeof(str), "%H:%M", localtime(&currTime));
    else
      strftime(str, sizeof(str), "%H %M", localtime(&currTime));

    it.print(19, 28, this->clock_font_, this->night_mode_state_ ? RED : LIGHT_GRAY, TextAlign::BASELINE_LEFT, str);
  }

  // --- Outside temp ---
  if (!std::isnan(this->temperature_outside_)) {
    auto outside_color = get_temp_color(this->temperature_outside_);
    it.print(
        2, 12, this->extra_font_, this->night_mode_state_ ? RED : outside_color, TextAlign::BASELINE_LEFT,
        ((this->temperature_outside_ > 0 ? "+" : "") + std::to_string((int) this->temperature_outside_) + "°").c_str());
  } else {
    it.print(2, 12, this->extra_font_, this->night_mode_state_ ? RED : YELLOW, TextAlign::BASELINE_LEFT, "---°");
  }

  // --- Weather icon ---
  if (!this->weather_icon_.empty()) {
    if (!this->night_mode_state_) {
      it.print(102, 24, this->icon_font_, ORANGE, TextAlign::BASELINE_LEFT, get_icon_char(this->weather_icon_));
    }
  }

  // --- Inside temp ---
  if (!std::isnan(this->temperature_inside_)) {
    auto inside_color = get_temp_color(this->temperature_inside_);
    it.print(
        2, 26, this->extra_font_, this->night_mode_state_ ? RED : inside_color, TextAlign::BASELINE_LEFT,
        ((this->temperature_inside_ > 0 ? "+" : "") + std::to_string((int) this->temperature_inside_) + "°").c_str());
  } else {
    it.print(2, 26, this->extra_font_, this->night_mode_state_ ? RED : YELLOW, TextAlign::BASELINE_LEFT, "---°");
  }

  draw_colored_line(it, this->temperature_progress_, this->night_mode_state_);

  for (Corner c : {Corner::TOP_LEFT, Corner::TOP_RIGHT, Corner::BOTTOM_RIGHT, Corner::BOTTOM_LEFT}) {
    if (get_corner_state(c)) {
      draw_alert_corner(it, c, RED);
    }
  }
}

void DisplayTools::render_app_screen(display::Display &it) {
  // ------------------------------
  // ТІЛЬКИ нижня частина (apps/alerts)
  // ------------------------------

  // Alerts мають пріоритет
  AlertMessage alert;
  if (this->getCurrentAlert(alert)) {
    if (first_alert_play_) {
      char *endptr = nullptr;
      long val = strtol(alert.sound.c_str(), &endptr, 10);

      if (endptr != alert.sound.c_str() && *endptr == '\0') {
        emit_on_play_sound(static_cast<int>(val));
      } else {
        ESP_LOGW("display_tools", "Invalid number string: '%s'", alert.sound.c_str());
      }

      first_alert_play_ = false;
    }
    bool done = false;
    if (alert.text.length() > 255) {
      done = this->drawPagedTextWithIcon(it, alert.text, alert.color, alert.icon, alert.icon_color, this->app_font_,
                                             this->icon_font_, alert.repeat);
    } else {
      done = this->drawScrollingTextWithIcon(it, alert.text, alert.color, alert.icon, alert.icon_color, this->app_font_,
                                         this->icon_font_, alert.repeat);
    }

    if (done) {
      this->removeCurrentAlert();
    }
    return;
  }

  if (this->night_mode_state_) {
    // Якщо нічний режим, то не показувати сповіщення
    it.print((it.get_width() / 2) - 10, 57, this->icon_font_, RED, TextAlign::BASELINE_LEFT,
             get_icon_char("mdi:bed-clock"));
    return;
  }

  // Якщо алертів нема → рендеримо apps
  App_Info *app = this->getCurrentApp();
  if (app != nullptr) {
    bool done = false;

    if (!app->text_parts.empty()) {
      done = this->drawScrollingTextWithIcon(it, app->text_parts, app->icon, app->icon_color, this->icon_font_,
                                             app->duration);
    } else if (!app->draw_objects.empty()) {
      done = this->drawDrawObjectsWithIcon(it, this->app_font_, app->draw_objects, app->icon, this->icon_font_,
                                           app->icon_color);
    } else {
      if (app->name == "__date__") {
        done = drawTodayDate(it, this->app_font_, 0, 52);
      } else {
        done = this->drawScrollingTextWithIcon(it, app->body, app->color, app->icon, app->icon_color, this->app_font_,
                                               this->icon_font_, app->duration);
      }
    }

    if (done) {
      this->nextApp();
    }
  }
}

std::vector<DisplayTools::ColoredWord> DisplayTools::make_colored_words(const std::vector<std::string> &texts,
                                                                        const std::vector<std::string> &colors,
                                                                        BaseFont *text_font, BaseFont *icon_font) {
  std::vector<ColoredWord> out;
  const std::string mdi_prefix = "mdi:";
  const size_t n = texts.size();
  out.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    const std::string &t = texts[i];
    const std::string &c = (i < colors.size()) ? colors[i] : std::string("FFFFFF");

    const bool is_icon = (t.rfind(mdi_prefix, 0) == 0);
    out.push_back(
        ColoredWord{is_icon ? std::string(get_icon_char(t)) : t, hex_to_color(c), is_icon ? icon_font : text_font});
  }
  return out;
}

}  // namespace display_tools
}  // namespace esphome