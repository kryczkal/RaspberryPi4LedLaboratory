#ifndef GPIO_WRAPPER_HPP
#define GPIO_WRAPPER_HPP

#include <gpio.h>
#include <string>
#include <stdexcept>
#include <cstdio> 


class GpioWrapper {
private:
    gpio_t* handle_ = nullptr;
    bool opened_ = false;
    std::string chip_path_;
    unsigned int line_;
    gpio_direction_t direction_;

    void handleError(const std::string& operation) const {
        if (handle_) {
            fprintf(stderr, "%s: %s (errno %d)\n",
                    operation.c_str(), gpio_errmsg(handle_), gpio_errno(handle_));
        } else {
            fprintf(stderr, "%s: Invalid GPIO handle\n", operation.c_str());
        }
        
         throw std::runtime_error(operation + " failed: " + (handle_ ? gpio_errmsg(handle_) : "Invalid handle"));
    }


public:
    GpioWrapper() : handle_(gpio_new()) {
        if (!handle_) {
            throw std::runtime_error("gpio_new() failed: Could not allocate GPIO handle.");
        }
    }

    
    GpioWrapper(const GpioWrapper&) = delete;
    GpioWrapper& operator=(const GpioWrapper&) = delete;

    
    GpioWrapper(GpioWrapper&& other) noexcept
        : handle_(other.handle_), opened_(other.opened_),
          chip_path_(std::move(other.chip_path_)), line_(other.line_),
          direction_(other.direction_)
    {
        other.handle_ = nullptr;
        other.opened_ = false;
    }

    GpioWrapper& operator=(GpioWrapper&& other) noexcept {
        if (this != &other) {
            close(); 
            if(handle_) gpio_free(handle_); 

            handle_ = other.handle_;
            opened_ = other.opened_;
            chip_path_ = std::move(other.chip_path_);
            line_ = other.line_;
            direction_ = other.direction_;

            other.handle_ = nullptr;
            other.opened_ = false;
        }
        return *this;
    }


    ~GpioWrapper() {
       close();
       if(handle_) {
           gpio_free(handle_);
           handle_ = nullptr;
       }
    }

    bool open(const std::string& chip_path, unsigned int line, gpio_direction_t direction) {
        if (opened_) {
            fprintf(stderr, "GPIO line %u already opened.\n", line_);
            return false; 
        }
         chip_path_ = chip_path;
         line_ = line;
         direction_ = direction;

        if (gpio_open(handle_, chip_path_.c_str(), line_, direction_) < 0) {
            handleError("gpio_open");
            opened_ = false;
            return false;
        }
        opened_ = true;
        return true;
    }

     bool open_advanced(const std::string& chip_path, unsigned int line, const gpio_config_t* config) {
        if (opened_) {
            fprintf(stderr, "GPIO line %u already opened.\n", line_);
            return false;
        }
        chip_path_ = chip_path;
        line_ = line;
        direction_ = config->direction; 

        if (gpio_open_advanced(handle_, chip_path_.c_str(), line_, config) < 0) {
            handleError("gpio_open_advanced");
            opened_ = false;
            return false;
        }
        opened_ = true;
        return true;
    }


    bool write(bool value) {
        if (!opened_) return false;
        if (gpio_write(handle_, value) < 0) {
            handleError("gpio_write");
            return false;
        }
        return true;
    }

    bool read(bool& value) {
        if (!opened_) return false;
        if (gpio_read(handle_, &value) < 0) {
            handleError("gpio_read");
            return false;
        }
        return true;
    }

     bool set_edge(gpio_edge_t edge) {
        if (!opened_) return false;
        if (gpio_set_edge(handle_, edge) < 0) {
            gpio_direction_t current_dir;
            if(gpio_get_direction(handle_, &current_dir) >= 0 && current_dir != GPIO_DIR_IN) {
                 fprintf(stderr, "Cannot set edge on non-input GPIO line %u.\n", line_);
                 return false; // Nie można ustawić zbocza dla wyjścia
            }

            handleError("gpio_set_edge");
            return false;
        }
        return true;
    }

      
    bool set_bias(gpio_bias_t bias) {
        if (!opened_) return false;
        if (gpio_set_bias(handle_, bias) < 0) {
            handleError("gpio_set_bias");
            return false;
        }
        return true;
    }

    int poll(int timeout_ms) {
        if (!opened_) return -1; 
        int ret = gpio_poll(handle_, timeout_ms);
        if (ret < 0) {
            handleError("gpio_poll");
        }
        return ret; 
    }

    bool read_event(gpio_edge_t& edge, uint64_t& timestamp) {
        if (!opened_) return false;
        if (gpio_read_event(handle_, &edge, &timestamp) < 0) {
            handleError("gpio_read_event");
            return false;
        }
        return true;
    }


    void close() {
        if (opened_ && handle_) {
            if (gpio_close(handle_) < 0) {
                 fprintf(stderr, "gpio_close failed for line %u: %s (errno %d)\n",
                         line_, gpio_errmsg(handle_), gpio_errno(handle_));
            }
             opened_ = false;
        }
    }

    gpio_t* getHandle() const { return handle_; }
    bool isOpened() const { return opened_; }
    unsigned int getLine() const { return line_; }
};

#endif // GPIO_WRAPPER_HPP
