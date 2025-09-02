#pragma once
#include "esphome.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace dfplayer_pro {

#define BUFFER_LENGTH 255

class DFPlayerPro : public esphome::Component, public uart::UARTDevice {
 public:
  // Конструктор, який приймає батьківський компонент UART
  DFPlayerPro(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

  void setup() override;
  void loop() override;

  void init();
  void send_cmd(const std::string cmd, const std::string value);

  void play_file_no(int no);
  void play_file(const std::string file_name);
  void set_prompt(bool on_off);
  void set_volume(int value);
  void set_play_mode(int mode);

 protected:
  void send_command(const std::string &cmd, std::function<void(bool)> cb = nullptr);
 private:
  struct PendingCommand {
    std::string cmd;
    std::function<void(bool)> callback;
    uint32_t start_time;
    bool sent{false};
  };

  std::deque<PendingCommand> queue_;
  PendingCommand *active_{nullptr};
  std::string response_buffer_;
  const uint32_t timeout_ms_ = 1000;

  void process_active();
  bool initialized_{false};
};

} // namespace dfplayer_pro
} // namespace esphome
