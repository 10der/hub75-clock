#include "dfplayer_pro.h"

namespace esphome {
namespace dfplayer_pro {

static const char *const TAG = "DFPlayerPro";

void DFPlayerPro::setup() {
  ESP_LOGD(TAG, "Налаштування компонента DFPlayerPro...");
  this->initialized_ = false;
}

void DFPlayerPro::init() {
  // Ініціалізація UART для DFPlayer Pro без затримки
  this->initialized_ = false;

  send_command("AT\r\n", [](bool ok) { ESP_LOGI("DFPlayer", "Wakeup: %s", ok ? "OK" : "ERR"); });

  send_command("AT+LED=OFF\r\n", [](bool ok) { ESP_LOGI("DFPlayer", "LED: %s", ok ? "OK" : "ERR"); });

  send_command("AT+PLAYMODE=3\r\n", [](bool ok) { ESP_LOGI("DFPlayer", "PlayMode: %s", ok ? "OK" : "ERR"); });

  send_command("AT+VOL=30\r\n", [](bool ok) { ESP_LOGI("DFPlayer", "Volume: %s", ok ? "OK" : "ERR"); });

  this->initialized_ = true;
}

void DFPlayerPro::send_command(const std::string &cmd, std::function<void(bool)> cb) {
  queue_.push_back({cmd, cb, 0, false});
}

/*
AT+BAUDRATE=<baudrate>: Sets the serial communication baud rate (e.g., AT+BAUDRATE=115200). This setting is saved and valid after re-powering.
AT+AMP=<ON/OFF>: Turns the amplifier on or off (e.g., AT+AMP=ON).
AT+PROMPT=<ON/OFF>: Turns the prompt tone on or off (e.g., AT+PROMPT=ON). This setting is saved. 
AT+LED=<ON/OFF>: Turns the LED prompt on or off (e.g., AT+LED=ON). This setting is saved. 
AT+PLAYFILE=/<path/filename.mp3>: Plays a specific file by its path and filename (e.g., AT+PLAYFILE=/MUSIC/song.mp3).
AT+PLAYNUM=<file_number>: Plays a file by its numerical index, based on the order files were copied (e.g., AT+PLAYNUM=1).
AT+DEL: Deletes the currently playing file.
AT+NEXT: Plays the next song in the current playlist.
AT+LAST: Plays the previous song in the current playlist.
AT+START: Starts playback (if paused).
AT+PAUSE: Pauses the current playback.
AT+FASTFORWARD=<seconds>: Fast forwards the current song by a specified number of seconds (e.g., AT+FASTFORWARD=10).
AT+FASTREVERSE=<seconds>: Fast reverses the current song by a specified number of seconds (e.g., AT+FASTREVERSE=5).
AT+GETVOLUME: Retrieves the current volume level.
AT+GETPLAYMODE: Retrieves the current playback mode.
AT+GETCURFILENUMBER: Retrieves the number of the currently playing file.
AT+GETTOTALFILE: Retrieves the total number of playable files.
AT+GETCURTIME: Retrieves the time length the current song has played.
AT+GETTOTALTIME: Retrieves the total length of the currently playing song.
AT+GETFILENAME: Retrieves the name of the currently playing file.
AT+SETVOLUME=<volume_level>: Sets the volume level (0-30).
AT+SETPLAYMODE=<mode>: Sets the playback mode (e.g., single play, loop).
*/

void DFPlayerPro::send_cmd(const std::string cmd, const std::string value) {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Компонент ще не ініціалізовано. Команда ігнорується.");
    return;
  }

   std::string atCmd = "";
   atCmd += "AT";
   if (!cmd.empty()) {
      atCmd += "+";
      atCmd += cmd;
   }

   if (!value.empty()) {
      atCmd += "=";
      atCmd += value;
   }
   atCmd += "\r\n";

  this->send_command(atCmd);
}

void DFPlayerPro::set_prompt(bool on_off) {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Компонент ще не ініціалізовано. Команда ігнорується.");
    return;
  }
  if (on_off) {
    this->send_command("AT+PROMPT=ON\r\n");
  } else {
    this->send_command("AT+PROMPT=OFF\r\n");
  }
}

void DFPlayerPro::play_file(const std::string file_name) {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Компонент ще не ініціалізовано. Команда ігнорується.");
    return;
  }
  this->send_command("AT+PLAYFILE=" + file_name + "\r\n");
}

void DFPlayerPro::play_file_no(int no) {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Компонент ще не ініціалізовано. Команда ігнорується.");
    return;
  }
  this->send_command("AT+PLAYNUM=" + std::to_string(no) + "\r\n");
}

void DFPlayerPro::set_volume(int value) {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Компонент ще не ініціалізовано. Команда ігнорується.");
    return;
  }
  this->send_command("AT+VOL=" + std::to_string(value) + "\r\n");
}

void DFPlayerPro::process_active() {
  // Читаємо все, що є в UART
  while (this->available()) {
    uint8_t c;
    this->read_byte(&c);
    response_buffer_ += (char) c;
    // Кінець відповіді?
    if (response_buffer_.size() >= 2 && response_buffer_[response_buffer_.size() - 2] == '\r' &&
        response_buffer_[response_buffer_.size() - 1] == '\n') {
      bool ok = response_buffer_.find("OK") != std::string::npos;
      ESP_LOGI(TAG, "DFPlayer response: %s", response_buffer_.c_str());

      if (active_->callback)
        active_->callback(ok);

      queue_.pop_front();  // видаляємо з черги
      active_ = nullptr;   // звільняємо слот
      return;
    }
  }

  // Таймаут
  if (millis() - active_->start_time > timeout_ms_) {
    ESP_LOGE(TAG, "Timeout for command: %s", active_->cmd.c_str());
    if (active_->callback)
      active_->callback(false);
    queue_.pop_front();
    active_ = nullptr;
  }
}

void DFPlayerPro::loop() {
  // Якщо немає активної команди і є в черзі — беремо її
  if (active_ == nullptr && !queue_.empty()) {
    active_ = &queue_.front();
    active_->start_time = millis();
    this->write_str(active_->cmd.c_str());
    ESP_LOGD(TAG, "Sent command: %s", active_->cmd.c_str());
    active_->sent = true;
    response_buffer_.clear();
  }

  if (active_ != nullptr) {
    process_active();
  }
}

/*
std::string DFPlayerPro::read_line(uint32_t timeout_ms) {
  std::string line;
  uint32_t start = millis();

  while (millis() - start < timeout_ms) {
    if (this->available()) {
      uint8_t c;
      if (this->read_byte(&c)) {
        line.push_back((char)c);

        // перевірка на CRLF
        if (line.size() >= 2 && line[line.size() - 2] == '\r' && line[line.size() - 1] == '\n') {
          // обрізаємо CRLF
          line.resize(line.size() - 2);
          return line;
        }
      }
    }
    // щоб не крутити CPU в холосту
    delay(1);
  }

  return "";  // таймаут -> пустий рядок
}

std::string DFPlayerPro::send_command_wait(const std::string &cmd, uint32_t timeout_ms) {
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Відправлено команду: '%s'", cmd.c_str());

  std::string resp = this->read_line(timeout_ms);
  if (resp.empty()) {
    ESP_LOGE(TAG, "Timeout waiting for response to: %s", cmd.c_str());
  } else {
    ESP_LOGD(TAG, "Отримано відповідь: '%s'", resp.c_str());
  }
  return resp;
}
*/

}  // namespace dfplayer_pro
}  // namespace esphome
