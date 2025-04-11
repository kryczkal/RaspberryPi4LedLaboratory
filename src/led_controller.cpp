#include "led_controller.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

LedController::LedController(const std::vector<unsigned int>& led_pins, const std::string& chip_path)
    : chip_path_(chip_path),
      current_pattern_index_(0),
      animation_delay_ms_(250),
      running_(false)
{
  if (led_pins.empty()) {
    throw std::invalid_argument("LED pins vector cannot be empty.");
  }

  leds_.reserve(led_pins.size());
  try {
    for (unsigned int pin : led_pins) {
      GpioWrapper led;
      if (!led.open(chip_path_, pin, GPIO_DIR_OUT)) {


        fprintf(stderr, "Failed to open LED GPIO %u on %s\n", pin, chip_path_.c_str());
        throw std::runtime_error("Failed to initialize one or more LEDs.");
      }

      led.write(false);
      leds_.push_back(std::move(led));
    }
  } catch (const std::exception& e) {


    fprintf(stderr, "Exception during LedController initialization: %s\n", e.what());
    throw;
  }

  std::cout << "LedController initialized with " << leds_.size() << " LEDs." << std::endl;
}

LedController::~LedController() {
  stopAnimation();
  turnOffAllLeds();

  std::cout << "LedController destroyed." << std::endl;
}

void LedController::addPattern(const LedPattern& pattern) {
  std::lock_guard<std::mutex> lock(pattern_mutex_);
  if (!pattern.empty() && !pattern[0].empty() && pattern[0].size() != leds_.size()) {
    fprintf(stderr, "Error adding pattern: Pattern step size (%zu) does not match number of LEDs (%zu).\n",
            pattern[0].size(), leds_.size());
    return;
  }
  patterns_.push_back(pattern);
  std::cout << "Added pattern. Total patterns: " << patterns_.size() << std::endl;

}

void LedController::nextPattern() {
  std::lock_guard<std::mutex> lock(pattern_mutex_);
  if (patterns_.empty()) return;
  current_pattern_index_ = (current_pattern_index_ + 1) % patterns_.size();
  std::cout << "Switched to next pattern: " << current_pattern_index_.load() << std::endl;
}

void LedController::previousPattern() {
  std::lock_guard<std::mutex> lock(pattern_mutex_);
  if (patterns_.empty()) return;
  current_pattern_index_ = (current_pattern_index_ - 1 + patterns_.size()) % patterns_.size();
  std::cout << "Switched to previous pattern: " << current_pattern_index_.load() << std::endl;
}

void LedController::increaseSpeed(int delta_ms) {
  int current_delay = animation_delay_ms_.load();
  int new_delay = std::max(min_delay_ms_, current_delay - delta_ms);
  animation_delay_ms_.store(new_delay);
  std::cout << "Animation speed increased. New delay: " << new_delay << " ms" << std::endl;
}

void LedController::decreaseSpeed(int delta_ms) {
  int current_delay = animation_delay_ms_.load();
  int new_delay = std::min(max_delay_ms_, current_delay + delta_ms);
  animation_delay_ms_.store(new_delay);
  std::cout << "Animation speed decreased. New delay: " << new_delay << " ms" << std::endl;
}

void LedController::startAnimation() {
  if (running_.load()) {
    return;
  }
  if (patterns_.empty()) {
    std::cerr << "Cannot start animation: No patterns added." << std::endl;
    return;
  }

  running_.store(true);

  animation_thread_ = std::thread(&LedController::animationLoop, this);
  std::cout << "Animation thread started." << std::endl;

}

void LedController::stopAnimation() {
  running_.store(false);
  if (animation_thread_.joinable()) {
    animation_thread_.join();
    std::cout << "Animation thread stopped." << std::endl;
  }
}

void LedController::animationLoop() {
  while (running_.load()) {
    LedPattern current_pattern;
    int pattern_idx;
    {
      std::lock_guard<std::mutex> lock(pattern_mutex_);
      if(patterns_.empty()) {

        running_.store(false);
        break;
      }
      pattern_idx = current_pattern_index_.load();

      current_pattern = patterns_[pattern_idx];
    }

    if (current_pattern.empty()) {

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }


    for (const auto& step : current_pattern) {
      if (!running_.load()) break;

      if(step.size() == leds_.size()) {
        setLeds(step);
      } else {
        fprintf(stderr, "Pattern %d step size mismatch (%zu vs %zu LEDs). Skipping step.\n",
                pattern_idx, step.size(), leds_.size());
      }


      std::this_thread::sleep_for(std::chrono::milliseconds(animation_delay_ms_.load()));
    }



  }
  turnOffAllLeds();
}

void LedController::setLeds(const LedStates& states) {
  for (size_t i = 0; i < leds_.size() && i < states.size(); ++i) {
    leds_[i].write(states[i]);
  }
}

void LedController::turnOffAllLeds() {
  std::vector<bool> off_states(leds_.size(), false);
  setLeds(off_states);
  std::cout << "Turned off all LEDs." << std::endl;
}