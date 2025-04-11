#include "led_controller.hpp"
#include "gpio_wrapper.hpp" // Potrzebujemy też dla przycisków
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <csignal> // Dla obsługi Ctrl+C
#include <atomic>
#include <iostream>
#include <map> // Do mapowania pinów przycisków na akcje
#include <functional> // Dla std::function

// --- Konfiguracja GPIO ---
const std::string GPIO_CHIP = "/dev/gpiochip0";
const std::vector<unsigned int> LED_PINS = {27, 23, 22, 24}; // D1, D2, D3, D4
const std::vector<unsigned int> BUTTON_PINS = {18, 17, 10, 25}; // SW1, SW2, SW3, SW4
const unsigned int BTN_PREV_PATTERN_PIN = BUTTON_PINS[0]; // SW1
const unsigned int BTN_NEXT_PATTERN_PIN = BUTTON_PINS[1]; // SW2
const unsigned int BTN_SPEED_DOWN_PIN = BUTTON_PINS[2];   // SW3 (Zwolnij -> zwiększ opóźnienie)
const unsigned int BTN_SPEED_UP_PIN = BUTTON_PINS[3];     // SW4 (Przyspiesz -> zmniejsz opóźnienie)


// Globalna flaga do obsługi sygnału przerwania (Ctrl+C)
std::atomic<bool> keep_running(true);

void signal_handler(int signum) {
    std::cout << "\nCaught signal " << signum << ". Shutting down..." << std::endl;
    keep_running.store(false);
}

// --- Definicje wzorców LED ---
const LedPattern pattern_blink_all = {
    {true, true, true, true},
    {false, false, false, false}
};

const LedPattern pattern_chase = {
    {true, false, false, false},
    {false, true, false, false},
    {false, false, true, false},
    {false, false, false, true},
    {false, false, true, false},
    {false, true, false, false}
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



int main() {
    // Zarejestruj obsługę sygnału SIGINT (Ctrl+C)
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler); // Obsługa sygnału zakończenia

    try {
        // --- Inicjalizacja Kontrolera LED ---
        LedController led_controller(LED_PINS, GPIO_CHIP);

        // Dodaj wzorce
        led_controller.addPattern(pattern_blink_all);
        led_controller.addPattern(pattern_chase);
         led_controller.addPattern(pattern_knight_rider);
         led_controller.addPattern(pattern_on_off_pairs);
         led_controller.addPattern(pattern_alternate);


        // --- Inicjalizacja Przycisków ---
        std::vector<GpioWrapper> buttons;
        std::vector<gpio_t*> button_handles; // Potrzebne dla gpio_poll_multiple
        std::map<unsigned int, std::function<void()>> button_actions;

        buttons.reserve(BUTTON_PINS.size());
        button_handles.reserve(BUTTON_PINS.size());

         // Konfiguracja dla przycisków (wejście, zbocze opadające, bez biasu - używamy zewnętrznych pull-upów)
         gpio_config_t button_config = {};
         button_config.direction = GPIO_DIR_IN;
         button_config.edge = GPIO_EDGE_FALLING; // Reaguj na wciśnięcie (zwarcie do GND)
         button_config.bias = GPIO_BIAS_DEFAULT; // Lub GPIO_BIAS_DISABLE, zewnętrzne pull-upy są obecne
         button_config.drive = GPIO_DRIVE_DEFAULT; // Nieistotne dla wejścia
         button_config.inverted = false;
         button_config.label = "LedProjectButton";


        for(unsigned int pin : BUTTON_PINS) {
            GpioWrapper button;
            // Otwórz zaawansowaną funkcją, aby ustawić zbocze od razu
            if (!button.open_advanced(GPIO_CHIP, pin, &button_config)) {
                 fprintf(stderr, "Failed to open button GPIO %u\n", pin);
                return 1; // Zakończ, jeśli nie można otworzyć przycisku
            }

             // Ustawienie zbocza (już zrobione w open_advanced)
            /*
            if (!button.set_edge(GPIO_EDGE_FALLING)) {
                 fprintf(stderr, "Failed to set edge detection for button GPIO %u\n", pin);
                 // Można kontynuować bez detekcji zbocza, ale poll nie zadziała poprawnie
                 // return 1;
            }
            */

            // Mapowanie akcji
            if (pin == BTN_PREV_PATTERN_PIN) button_actions[pin] = [&]() { led_controller.previousPattern(); };
            else if (pin == BTN_NEXT_PATTERN_PIN) button_actions[pin] = [&]() { led_controller.nextPattern(); };
            else if (pin == BTN_SPEED_DOWN_PIN) button_actions[pin] = [&]() { led_controller.decreaseSpeed(); };
            else if (pin == BTN_SPEED_UP_PIN) button_actions[pin] = [&]() { led_controller.increaseSpeed(); };

            button_handles.push_back(button.getHandle()); // Dodaj surowy uchwyt do listy dla poll_multiple
            buttons.push_back(std::move(button)); // Przenieś GpioWrapper
        }
         std::cout << "Buttons initialized." << std::endl;


        // --- Uruchomienie Animacji ---
        led_controller.startAnimation();

        // --- Pętla Obsługi Przycisków ---
         std::cout << "Starting button polling loop. Press Ctrl+C to exit." << std::endl;
         std::chrono::milliseconds last_press_time = std::chrono::milliseconds(0);
         const std::chrono::milliseconds debounce_delay(200); // Czas anty-drganiowy

        while (keep_running.load()) {
            bool gpios_ready[BUTTON_PINS.size()]; // Tablica wyników dla gpio_poll_multiple

             // Czekaj na zdarzenie na którymkolwiek przycisku przez 100ms
            int ret = gpio_poll_multiple(button_handles.data(), button_handles.size(), 100, gpios_ready);

            if (ret < 0) {
                 // Błąd w gpio_poll_multiple - może wymagać ponownej inicjalizacji?
                 fprintf(stderr, "gpio_poll_multiple() error: %s\n", gpio_errmsg(button_handles[0])); // Pokaż błąd z pierwszego uchwytu
                 // Można spróbować odczekać chwilę i kontynuować
                 std::this_thread::sleep_for(std::chrono::milliseconds(500));
                 continue;
             } else if (ret > 0) {
                 // Któryś przycisk wygenerował zdarzenie
                 auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch()
                           );

                  // Prosty globalny debounce - ignoruj kolejne wciśnięcia przez krótki czas
                 if (now - last_press_time < debounce_delay) {
                     // Zbyt szybko po ostatnim wciśnięciu, zignoruj (można oczyścić zdarzenie?)
                      // Przeczytaj zdarzenia, aby je wyczyścić, ale nie reaguj
                     for (size_t i = 0; i < button_handles.size(); ++i) {
                         if (gpios_ready[i]) {
                            gpio_edge_t edge;
                            uint64_t timestamp;
                            // Przeczytaj zdarzenie, aby je usunąć z kolejki kernela
                            if (gpio_read_event(button_handles[i], &edge, &timestamp) < 0) {
                                 fprintf(stderr, "Error reading event after debounce on GPIO %u: %s\n",
                                         gpio_line(button_handles[i]), gpio_errmsg(button_handles[i]));
                            }
                         }
                     }
                     continue;
                 }


                 last_press_time = now; // Zaktualizuj czas ostatniego wciśnięcia

                 for (size_t i = 0; i < button_handles.size(); ++i) {
                     if (gpios_ready[i]) {
                         unsigned int line = gpio_line(button_handles[i]); // Pobierz numer linii GPIO
                         std::cout << "Button event detected on GPIO " << line << std::endl;

                         // Przeczytaj zdarzenie (aby je "skonsumować" w kernelu)
                         gpio_edge_t edge;
                         uint64_t timestamp;
                         if (gpio_read_event(button_handles[i], &edge, &timestamp) < 0) {
                              fprintf(stderr, "gpio_read_event() error on GPIO %u: %s\n", line, gpio_errmsg(button_handles[i]));
                         } else {
                              // Sprawdź, czy to zbocze opadające (wciśnięcie)
                              // W cdev v1/v2, poll budzi się na zdarzenie, a read_event daje szczegóły
                             if (edge == GPIO_EDGE_FALLING) { // Reaguj tylko na wciśnięcie
                                 if (button_actions.count(line)) {
                                     button_actions[line](); // Wywołaj odpowiednią akcję
                                 }
                             }
                         }

                          // Dodatkowy mały sleep debounce per przycisk? Raczej niepotrzebny przy globalnym.
                          // std::this_thread::sleep_for(debounce_delay);
                     }
                 }
             }
             // Jeśli ret == 0, upłynął timeout, nic się nie stało, kontynuuj pętlę
        }

        // --- Sprzątanie ---
        std::cout << "Stopping animation..." << std::endl;
        led_controller.stopAnimation(); // Zatrzymaj wątek animacji przed wyjściem

        std::cout << "Closing GPIOs..." << std::endl;
        // GpioWrapper RAII zajmie się zamknięciem i zwolnieniem GPIOs (LEDy i przyciski)
        // gdy obiekty wyjdą poza zakres (na końcu main)

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Led-Project finished gracefully." << std::endl;
    return 0;
}
