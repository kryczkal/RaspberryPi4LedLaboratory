#include "gpio_wrapper.hpp"
#include "led_controller.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <gpio.h>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

static constexpr std::string GPIO_CHIP = "/dev/gpiochip0";
const std::vector<unsigned int> LED_PINS = {27, 23, 22, 24};
const std::vector<unsigned int> BUTTON_PINS = {18, 17, 10, 25};
const unsigned int BTN_PREV_PATTERN_PIN = BUTTON_PINS[0];
const unsigned int BTN_NEXT_PATTERN_PIN = BUTTON_PINS[1];
const unsigned int BTN_SPEED_DOWN_PIN = BUTTON_PINS[2];
const unsigned int BTN_SPEED_UP_PIN = BUTTON_PINS[3];
const int POLL_TIMEOUT_MS = 100;
const std::chrono::milliseconds DEBOUNCE_DELAY_MS(200);

std::atomic<bool> keep_running(true);

void signal_handler(int signum) {
  std::cout << "\nCaught signal " << signum << ". Shutting down..."
            << std::endl;
  keep_running.store(false);
}

// clang-format off
const LedPattern pattern_blink_all = {
    {true, true, true, true},
    {false, false, false, false}
};

const LedPattern pattern_chase = {
    {true, false, false, false},
    {false, true, false, false},
    {false, false, true, false},
    {false, false, false, true},
};

const LedPattern pattern_knight_rider = {
    {true, false, false, false},
    {false, true, false, false},
    {false, false, true, false},
    {false, false, false, true},
    {false, false, true, false},
    {false, true, false, false}
};

const LedPattern pattern_on_off_pairs = {
    {true, true, false, false},
    {false, false, true, true}
};

const LedPattern pattern_alternate = {
    {true, false, true, false},
    {false, true, false, true}
};
// clang-format on

int main() {

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    LedController led_controller(LED_PINS, GPIO_CHIP);

    led_controller.addPattern(pattern_blink_all);
    led_controller.addPattern(pattern_chase);
    led_controller.addPattern(pattern_knight_rider);
    led_controller.addPattern(pattern_on_off_pairs);
    led_controller.addPattern(pattern_alternate);

    std::vector<GpioWrapper> buttons;
    std::vector<gpio_t *> button_handles;
    std::map<unsigned int, std::function<void()>> button_actions;

    buttons.reserve(BUTTON_PINS.size());
    button_handles.reserve(BUTTON_PINS.size());

    gpio_config_t button_config = {};
    button_config.direction = GPIO_DIR_IN;
    button_config.edge = GPIO_EDGE_FALLING;
    button_config.bias = GPIO_BIAS_DEFAULT;
    button_config.drive = GPIO_DRIVE_DEFAULT;
    button_config.inverted = false;
    button_config.label = "LedProjectButton";

    for (unsigned int pin : BUTTON_PINS) {
      GpioWrapper button;

      if (!button.open_advanced(GPIO_CHIP, pin, &button_config)) {
        fprintf(stderr, "Failed to open button GPIO %u\n", pin);
        return 1;
      }

      if (pin == BTN_PREV_PATTERN_PIN)
        button_actions[pin] = [&]() { led_controller.previousPattern(); };
      else if (pin == BTN_NEXT_PATTERN_PIN)
        button_actions[pin] = [&]() { led_controller.nextPattern(); };
      else if (pin == BTN_SPEED_DOWN_PIN)
        button_actions[pin] = [&]() { led_controller.decreaseSpeed(); };
      else if (pin == BTN_SPEED_UP_PIN)
        button_actions[pin] = [&]() { led_controller.increaseSpeed(); };

      button_handles.push_back(button.getHandle());
      buttons.push_back(std::move(button));
    }
    std::cout << "Buttons initialized." << std::endl;

    led_controller.startAnimation();

    std::cout << "Starting button polling loop. Press Ctrl+C to exit."
              << std::endl;
    std::chrono::milliseconds last_press_time = std::chrono::milliseconds(0);

    while (keep_running.load()) {
      bool gpios_ready[BUTTON_PINS.size()];

      int ret = gpio_poll_multiple(button_handles.data(), button_handles.size(),
                                   POLL_TIMEOUT_MS, gpios_ready);

      if (ret < 0) {
        std::cerr << "gpio_poll_multiple() error: "
                  << gpio_errmsg(button_handles[0]) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      } else if (ret > 0) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
        if (now - last_press_time < DEBOUNCE_DELAY_MS) {
          for (size_t i = 0; i < button_handles.size(); ++i) {
            if (gpios_ready[i]) {
              gpio_edge_t edge;
              uint64_t timestamp;

              if (gpio_read_event(button_handles[i], &edge, &timestamp) < 0) {
                std::cerr << "Error reading event after debounce on GPIO "
                          << gpio_line(button_handles[i]) << ": "
                          << gpio_errmsg(button_handles[i]) << std::endl;
              }
            }
          }
          last_press_time = now;
          continue;
        }
        last_press_time = now;

        for (size_t i = 0; i < button_handles.size(); ++i) {
          if (gpios_ready[i]) {
            unsigned int line = gpio_line(button_handles[i]);
            std::cout << "Button event detected on GPIO " << line << std::endl;

            gpio_edge_t edge;
            uint64_t timestamp;
            if (gpio_read_event(button_handles[i], &edge, &timestamp) < 0) {
              std::cerr << "Error reading event on GPIO " << line << ": "
                        << gpio_errmsg(button_handles[i]) << std::endl;
            } else {
              if (edge == GPIO_EDGE_FALLING) {
                if (button_actions.count(line)) {
                  button_actions[line]();
                }
              }
            }
          }
        }
      }
    }

    std::cout << "Stopping animation..." << std::endl;
    led_controller.stopAnimation();
    std::cout << "Closing GPIOs..." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Runtime Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Led-Project finished gracefully." << std::endl;
  return 0;
}
