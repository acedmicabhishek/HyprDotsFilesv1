#include "system.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <gtk/gtk.h>
#include <fstream>
#include <sstream>

static const char* temp_class(float temp) {
    return temp < 50.0f ? "stat-good" : temp < 70.0f ? "stat-moderate" : temp < 85.0f ? "stat-warn" : "stat-hot";
}

static const char* battery_class(int pct) {
    return pct >= 80 ? "stat-good" : pct >= 50 ? "stat-moderate" : pct >= 20 ? "stat-warn" : "stat-hot";
}

static const char* volume_class(float pct, bool muted) {
    if (muted) return "stat-muted";
    return pct < 0.30f ? "stat-good" : pct < 0.70f ? "stat-moderate" : pct < 0.90f ? "stat-warn" : "stat-hot";
}

static const char* net_class(long dr, long dt) {
    long total = dr + dt;
    return total == 0 ? "stat-muted" : total < 1024 ? "stat-moderate" : "stat-good";
}

static const char* wifi_class(const std::string &state, const std::string &ssid) {
    if (state.find("enabled") != std::string::npos) {
        return ssid.empty() ? "stat-moderate" : "stat-good";
    }
    if (state.find("disabled") != std::string::npos) return "stat-muted";
    return "stat-muted";
}

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
    if (App.modules["temp"]) {
      float temp_val = 0.0f;
      if (sscanf(temp.c_str(), "%f", &temp_val) == 1) {
        set_widget_stat_class(App.modules["temp"], temp_class(temp_val));
      } else {
        set_widget_stat_class(App.modules["temp"], "stat-moderate");
      }
    }
    safe_set_label(App.modules["bat"], bat);
    if (App.modules["bat"]) {
      int bat_pct = -1;
      if (sscanf(bat.c_str(), "%*[^0-9]%d", &bat_pct) == 1) {
        set_widget_stat_class(App.modules["bat"], battery_class(bat_pct));
        if (App.battery_label) {
          char bat_text[32];
          snprintf(bat_text, sizeof(bat_text), "Battery %d%%", bat_pct);
          safe_set_label(App.battery_label, bat_text);
        }
        if (App.battery_bar) {
          gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(App.battery_bar), bat_pct / 100.0);
        }
      } else {
        set_widget_stat_class(App.modules["bat"], "stat-moderate");
        if (App.battery_label) safe_set_label(App.battery_label, "Battery N/A");
      }
    }
  } catch (...) {}
}

void update_net() {
  try {
    std::ifstream net_file("/proc/net/dev");
    std::string line;
    long rx = 0, tx = 0;
    while (std::getline(net_file, line)) {
      if (line.find(":") == std::string::npos) continue;
      std::string iface = line.substr(0, line.find(":"));
      while (!iface.empty() && iface.front() == ' ') iface.erase(0, 1);
      if (iface == "lo") continue;
      std::istringstream iss(line.substr(line.find(":") + 1));
      long r, t;
      iss >> r;
      for (int i = 0; i < 7; ++i) iss >> std::ws >> t; // skip fields until tx_bytes
      iss >> t;
      rx += r;
      tx += t;
    }
    if (rx == 0 && tx == 0) {
      safe_set_label(App.modules["net"], "󰖩 N/A");
      if (App.modules["net"]) set_widget_stat_class(App.modules["net"], "stat-muted");
      return;
    }

    long dr = 0, dt = 0;
    if (App.net_last_recv > 0 && App.net_last_sent > 0) {
      dr = rx - App.net_last_recv;
      dt = tx - App.net_last_sent;
    }
    App.net_last_recv = rx;
    App.net_last_sent = tx;

    char buf[64];
    if (dr > 0 || dt > 0) {
      const char *down = dr > 0 ? "↓" : "";
      const char *up = dt > 0 ? "↑" : "";
      snprintf(buf, sizeof(buf), "󰤯 %s%.1fK %s%.1fK", down, dr / 1024.0, up, dt / 1024.0);
    } else {
      snprintf(buf, sizeof(buf), "󰤯 waiting");
    }
    safe_set_label(App.modules["net"], buf);
    if (App.modules["net"]) {
      set_widget_stat_class(App.modules["net"], net_class(dr, dt));
    }
  } catch (...) {
    safe_set_label(App.modules["net"], "󰖩 N/A");
    if (App.modules["net"]) set_widget_stat_class(App.modules["net"], "stat-muted");
  }
}

void update_wifi() {
  try {
    std::string wifi_state = exec("nmcli -t -f WIFI g 2>/dev/null");
    std::string label;
    std::string ssid;
    if (wifi_state.find("enabled") != std::string::npos) {
      ssid = exec("nmcli -t -f active,ssid dev wifi 2>/dev/null | awk -F: '$1==\"yes\" {print $2; exit}'");
      if (ssid.empty()) ssid = "scanning";
      label = "󰤨 " + ssid;
    } else if (wifi_state.find("disabled") != std::string::npos) {
      label = "󰤭 off";
    } else {
      label = "󰖩 N/A";
    }
    safe_set_label(App.modules["wifi"], label);
    if (App.modules["wifi"]) {
      set_widget_stat_class(App.modules["wifi"], wifi_class(wifi_state, ssid));
    }
  } catch (...) {
    safe_set_label(App.modules["wifi"], "󰖩 N/A");
    if (App.modules["wifi"]) set_widget_stat_class(App.modules["wifi"], "stat-muted");
  }
}

void update_volume() {
  try {
    std::string vol_out = exec("wpctl get-volume @DEFAULT_AUDIO_SINK@");
    float vol_val = 0.0;
    bool muted = vol_out.find("MUTED") != std::string::npos;
    if (sscanf(vol_out.c_str(), "Volume: %f", &vol_val) == 1) {
        int vol_pct = (int)(vol_val * 100);
        char vol_buf[32];
        snprintf(vol_buf, sizeof(vol_buf), "%s %d%%", muted ? "󰝟" : "󰕾", vol_pct);
        safe_set_label(App.modules["vol"], vol_buf);
        if (App.volume_label) {
          char drawer_vol[32];
          snprintf(drawer_vol, sizeof(drawer_vol), "Volume %d%%", muted ? 0 : vol_pct);
          safe_set_label(App.volume_label, drawer_vol);
        }
        if (App.volume_bar) {
          gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(App.volume_bar), muted ? 0.0 : vol_val);
        }
        if (App.modules["vol"]) {
          set_widget_stat_class(App.modules["vol"], volume_class(vol_val, muted));
        }
    }
  } catch (...) {}
}

void update_time() {
  try {
    std::string raw_time = exec("date +%H:%M:%S");
    std::string raw_date = exec("date '+%A, %d %B'");
    safe_set_label(App.big_clock, raw_time);
    safe_set_label(App.big_date, raw_date);
  } catch (...) {}
}
