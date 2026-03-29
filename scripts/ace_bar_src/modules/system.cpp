#include "system.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <gtk/gtk.h>

void update_system() {
  try {
    std::string active = exec("hyprctl activewindow -j");
    if (active.find("title") != std::string::npos) {
      size_t start = active.find("\"title\": \"") + 10;
      size_t end = active.find("\"", start);
      std::string title = active.substr(start, end - start);
      safe_set_label(App.title_label, "聚焦: " + title);
      safe_set_label(App.active_win_drawer, "Focused: " + title);
    } else {
      safe_set_label(App.title_label, "Desktop");
      safe_set_label(App.active_win_drawer, "Focused: Desktop");
    }

    std::string status = exec("AAC --status");
    char p_buf[32], g_buf[32];
    snprintf(p_buf, sizeof(p_buf), "󰈐 %s",
             (status.find("Silent") != std::string::npos) ? "Silent" : (status.find("Turbo") != std::string::npos ? "Turbo" : "Balanced"));
    snprintf(g_buf, sizeof(g_buf), "󰢮 %s",
             (status.find("Eco") != std::string::npos) ? "Eco" : (status.find("Nvidia") != std::string::npos ? "Nvidia" : "Hybrid"));
    safe_set_label(App.modules["power"], p_buf);
    safe_set_label(App.modules["gpu"], g_buf);

    std::string temp = exec("~/.config/hypr/scripts/sys_data.sh temp");
    std::string bat = exec("~/.config/hypr/scripts/sys_data.sh bat");
    safe_set_label(App.modules["temp"], " " + temp);
    safe_set_label(App.modules["bat"], bat);
  } catch (...) {}
}

gboolean update_clock(gpointer data) {
  try {
    std::string raw_time = exec("date +%H:%M:%S");
    std::string raw_date = exec("date '+%A, %d %B'");
    std::string bar_time = " " + raw_time.substr(0, 5) + "  󰃭 " + raw_date.substr(raw_date.find(",") + 2);
    safe_set_label(App.clock_label, bar_time);
    safe_set_label(App.big_clock, raw_time);
    safe_set_label(App.big_date, raw_date);

    std::string vol_out = exec("wpctl get-volume @DEFAULT_AUDIO_SINK@");
    float vol_val = 0.0;
    bool muted = vol_out.find("MUTED") != std::string::npos;
    if (sscanf(vol_out.c_str(), "Volume: %f", &vol_val) == 1) {
        char vol_buf[32];
        snprintf(vol_buf, sizeof(vol_buf), "%s %d%%", muted ? "󰝟" : "󰕾", (int)(vol_val * 100));
        safe_set_label(App.modules["vol"], vol_buf);
    }
  } catch (...) {}
  return TRUE;
}
