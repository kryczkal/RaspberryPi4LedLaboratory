#include <gpio.h>
#include <led.h>

#include <atomic>
#include <iostream>
#include <string>

int main() {
  gpio_t *gpio_led;
  gpio_led = gpio_new();

  // Open GPIO line 27 (connected to RED LED D1) as output
  if (gpio_open(gpio_led, "/dev/gpiochip0", 27, GPIO_DIR_OUT) < 0) {
    fprintf(stderr, "gpio_open(): %s\n", gpio_errmsg(gpio_led));
    exit(1);
  }

  for (int i = 0; i < 10; i++) {
    // Turn RED LED ON (set GPIO high)
    if (gpio_write(gpio_led, true) < 0) {
      fprintf(stderr, "gpio_write(): %s\n", gpio_errmsg(gpio_led));
      gpio_close(gpio_led);
      exit(1);
    }

    usleep(1000000); // Sleep for 1 second

    // Turn RED LED OFF (set GPIO low)
    if (gpio_write(gpio_led, false) < 0) {
      fprintf(stderr, "gpio_write(): %s\n", gpio_errmsg(gpio_led));
      gpio_close(gpio_led);
      exit(1);
    }
  }

  return 0;
}