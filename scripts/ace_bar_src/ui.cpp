#include "ui.hpp"
#include "globals.hpp"
#include "utils.hpp"
#include <gtk4-layer-shell.h>

void toggle_visibility() {
  if (App.is_visible) {
    gtk_widget_set_visible(App.win, FALSE);
    gtk_widget_set_visible(App.drawer_center, FALSE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(App.win), 0);
  } else {
    gtk_widget_set_visible(App.win, TRUE);
    gtk_widget_set_visible(App.drawer_center, TRUE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(App.win), 30);
  }
  App.is_visible = !App.is_visible;
}

void setup_layer(GtkWindow *win, bool is_bar, const char *pos) {
  gtk_layer_init_for_window(win);
  gtk_layer_set_layer(win, GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  if (is_bar) {
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_exclusive_zone(win, 30);
  } else {
    gtk_layer_set_namespace(win, "ace-drawer");
    if (strcmp(pos, "center") == 0) {
      gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
      gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    }
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_TOP, 42);
  }
}

void setup_ui(GtkApplication *app) {
  App.win = gtk_application_window_new(app);
  setup_layer(GTK_WINDOW(App.win), true, "");
  GtkWidget *mb = gtk_center_box_new();
  gtk_widget_add_css_class(mb, "bar-container");
  gtk_window_set_child(GTK_WINDOW(App.win), mb);

  GtkWidget *wsb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_add_css_class(wsb, "workspace-island");
  gtk_center_box_set_start_widget(GTK_CENTER_BOX(mb), wsb);
  for (int i = 1; i <= 10; ++i) {
    char l[4];
    snprintf(l, 4, "%d", i);
    GtkWidget *b = gtk_button_new_with_label(l);
    gtk_widget_add_css_class(b, "ws-btn");
    g_signal_connect_swapped(b, "clicked", G_CALLBACK(+[](GtkWidget *btn) {
        std::string ws = gtk_button_get_label(GTK_BUTTON(btn));
        system(("hyprctl dispatch workspace " + ws).c_str());
    }), b);
    gtk_box_append(GTK_BOX(wsb), b);
    App.ws_buttons[i] = b;
  }

  GtkWidget *cb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_add_css_class(cb, "island");
  gtk_center_box_set_center_widget(GTK_CENTER_BOX(mb), cb);
  App.title_label = gtk_label_new("Desktop");
  gtk_widget_add_css_class(App.title_label, "window-title");
  gtk_label_set_ellipsize(GTK_LABEL(App.title_label), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars(GTK_LABEL(App.title_label), 30);
  gtk_box_append(GTK_BOX(cb), App.title_label);

  GtkWidget *rb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(rb, "island");
  gtk_center_box_set_end_widget(GTK_CENTER_BOX(mb), rb);
  const char *mods[] = {"net", "wifi", "gpu", "power", "vol", "cpu", "mem", "temp", "bat"};
  for (auto m : mods) {
    if (strcmp(m, "gpu") == 0 || strcmp(m, "power") == 0 || strcmp(m, "wifi") == 0) {
        App.modules[m] = gtk_button_new_with_label("...");
        gtk_widget_add_css_class(App.modules[m], "ws-btn");
        
        g_signal_connect(App.modules[m], "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
            std::string mod = (char*)data;
            if (mod == "gpu") {
                safe_set_label(App.modules["gpu"], "󰢮 Wait...");
                system("~/.config/hypr/scripts/aac_cycle.sh gpu > /dev/null 2>&1 &");
            } else if (mod == "power") {
                safe_set_label(App.modules["power"], "󰈐 Wait...");
                system("~/.config/hypr/scripts/aac_cycle.sh power > /dev/null 2>&1 &");
            } else if (mod == "wifi") {
                safe_set_label(App.modules["wifi"], "󰤭 toggling...");
                system("sh -c 'if [ \"$(nmcli -t -f WIFI g 2>/dev/null)\" = \"enabled\" ]; then nmcli radio wifi off >/dev/null 2>&1; else nmcli radio wifi on >/dev/null 2>&1; fi' &");
            }
        }), strdup(m));
    } else {
        App.modules[m] = gtk_label_new("...");
    }

    char cls[32];
    snprintf(cls, 32, "mod-%s", m);
    gtk_widget_add_css_class(App.modules[m], cls);
    gtk_box_append(GTK_BOX(rb), App.modules[m]);
  }

  App.drawer_center = gtk_application_window_new(app);
  setup_layer(GTK_WINDOW(App.drawer_center), false, "center");
  GtkWidget *dc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_add_css_class(dc_box, "drawer-container");
  gtk_widget_add_css_class(dc_box, "drawer-center");
  gtk_window_set_child(GTK_WINDOW(App.drawer_center), dc_box);
  App.big_clock = gtk_label_new("00:00:00");
  gtk_widget_add_css_class(App.big_clock, "drawer-big-clock");
  gtk_box_append(GTK_BOX(dc_box), App.big_clock);
  App.big_date = gtk_label_new("Loading...");
  gtk_widget_add_css_class(App.big_date, "drawer-date");
  gtk_box_append(GTK_BOX(dc_box), App.big_date);
  App.weather_label = gtk_label_new("Fetching...");
  gtk_widget_add_css_class(App.weather_label, "drawer-weather");
  gtk_box_append(GTK_BOX(dc_box), App.weather_label);
  App.active_win_drawer = gtk_label_new("Focus: Desktop");
  gtk_widget_add_css_class(App.active_win_drawer, "drawer-title-large");
  gtk_label_set_ellipsize(GTK_LABEL(App.active_win_drawer), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars(GTK_LABEL(App.active_win_drawer), 30);
  gtk_box_append(GTK_BOX(dc_box), App.active_win_drawer);

  GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_add_css_class(status_box, "drawer-status-group");

  App.volume_label = gtk_label_new("Volume 0%");
  gtk_widget_add_css_class(App.volume_label, "drawer-stat-detail");
  gtk_box_append(GTK_BOX(status_box), App.volume_label);
  App.volume_bar = gtk_progress_bar_new();
  gtk_widget_add_css_class(App.volume_bar, "drawer-vol-bar");
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(App.volume_bar), TRUE);
  gtk_box_append(GTK_BOX(status_box), App.volume_bar);

  App.battery_label = gtk_label_new("Battery 0%");
  gtk_widget_add_css_class(App.battery_label, "drawer-stat-detail");
  gtk_box_append(GTK_BOX(status_box), App.battery_label);
  App.battery_bar = gtk_progress_bar_new();
  gtk_widget_add_css_class(App.battery_bar, "drawer-bat-bar");
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(App.battery_bar), TRUE);
  gtk_box_append(GTK_BOX(status_box), App.battery_bar);

  gtk_box_append(GTK_BOX(dc_box), status_box);

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(provider, CSS_FILE);
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  system("hyprctl keyword layerrule \"blur, ace-drawer\" >/dev/null 2>&1");
  system("hyprctl keyword layerrule \"ignorezero, ace-drawer\" >/dev/null 2>&1");

  gtk_window_present(GTK_WINDOW(App.drawer_center));
  gtk_window_present(GTK_WINDOW(App.win));
}
