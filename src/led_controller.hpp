#ifndef LED_CONTROLLER_HPP
#define LED_CONTROLLER_HPP

#include "gpio_wrapper.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional> 

using LedStates = std::vector<bool>;
using LedPattern = std::vector<LedStates>;

class LedController {
public:
    LedController(const std::vector<unsigned int>& led_pins, const std::string& chip_path = "/dev/gpiochip0");
    ~LedController();


    LedController(const LedController&) = delete;
    LedController& operator=(const LedController&) = delete;

    void addPattern(const LedPattern& pattern);
    void nextPattern();
    void previousPattern();
    void increaseSpeed(int delta_ms = 50); 
    void decreaseSpeed(int delta_ms = 50); 

    void startAnimation();
    void stopAnimation();


private:
    std::string chip_path_;
    std::vector<GpioWrapper> leds_;
    std::vector<LedPattern> patterns_;

    std::atomic<int> current_pattern_index_;
    std::atomic<int> animation_delay_ms_; 
    const int min_delay_ms_ = 50;  
    const int max_delay_ms_ = 1000; 

    std::thread animation_thread_;
    std::atomic<bool> running_;
    std::mutex pattern_mutex_; 

    void animationLoop();
    void setLeds(const LedStates& states);
    void turnOffAllLeds();
};

#endif // LED_CONTROLLER_HPP
