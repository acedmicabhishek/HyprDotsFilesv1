#pragma once

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <vector>

#define LOCK_FILE "/tmp/ace_bar.lock"
#define CSS_FILE "/home/light/.config/hypr/scripts/ace_bar.css"

struct AppState {
  bool is_visible = true;
  long net_last_recv = 0;
  long net_last_sent = 0;
  int cpu_count = 0;
  std::string weather_data = "Loading...";
  std::map<int, GtkWidget *> ws_buttons;
  std::map<std::string, GtkWidget *> modules;
  GtkWidget *win, *drawer_center;
  GtkWidget *big_clock, *big_date;
  GtkWidget *weather_label, *active_win_drawer;
  GtkWidget *volume_label, *battery_label;
  GtkWidget *volume_bar, *battery_bar;
  GtkWidget *title_label, *media_label;
  std::vector<std::vector<long>> last_cpu_times;
};

extern AppState App;
