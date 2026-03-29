#include "weather.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <thread>
#include <chrono>
#include <gtk/gtk.h>

void weather_loop() {
  while (true) {
    try {
      std::string out = exec("curl -s -m 5 'wttr.in?format=%t+%C+%w'");
      if (!out.empty() && out.find("+") != std::string::npos && out.length() < 50) {
        App.weather_data = "󰖐 " + out;
        g_idle_add(
            [](gpointer d) -> gboolean {
              safe_set_label(App.weather_label, App.weather_data);
              return FALSE;
            },
            NULL);
      }
    } catch (...) {}
    std::this_thread::sleep_for(std::chrono::minutes(10));
  }
}
