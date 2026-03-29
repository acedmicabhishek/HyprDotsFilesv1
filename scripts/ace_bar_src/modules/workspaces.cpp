#include "workspaces.hpp"
#include "../globals.hpp"
#include "../utils.hpp"

void update_workspaces() {
  try {
    std::string ws_info = exec("hyprctl workspaces -j");
    std::string active_ws_j = exec("hyprctl activeworkspace -j");
    int active_ws = 0;
    size_t id_pos = active_ws_j.find("\"id\": ");
    if (id_pos != std::string::npos) {
        active_ws = std::stoi(active_ws_j.substr(id_pos + 6));
    }

    for (int i = 1; i <= 10; ++i) {
      if (i == active_ws) {
          gtk_widget_add_css_class(App.ws_buttons[i], "active");
      } else {
          gtk_widget_remove_css_class(App.ws_buttons[i], "active");
      }
    }
  } catch (...) {}
}
