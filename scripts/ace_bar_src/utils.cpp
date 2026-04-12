#include "utils.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

std::string exec(const char *cmd) {
  char buffer[512];
  std::string result = "";
  FILE *pipe = popen(cmd, "r");
  if (!pipe) return "N/A";
  try {
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
      result += buffer;
    }
  } catch (...) {
    pclose(pipe);
    return "";
  }
  pclose(pipe);
  if (!result.empty() && result.back() == '\n') result.pop_back();
  return result;
}

void safe_set_label(GtkWidget* widget, const std::string& text) {
    if (!widget) return;
    try {
        const char *safe_text = g_utf8_validate(text.c_str(), -1, NULL) ? text.c_str() : "...";
        if (GTK_IS_LABEL(widget)) {
            gtk_label_set_text(GTK_LABEL(widget), safe_text);
        } else if (GTK_IS_BUTTON(widget)) {
            gtk_button_set_label(GTK_BUTTON(widget), safe_text);
        }
    } catch (...) {}
}

void set_widget_stat_class(GtkWidget* widget, const std::string& cls) {
    if (!widget) return;
    GtkStyleContext *ctx = gtk_widget_get_style_context(widget);
    const char *classes[] = {"stat-muted", "stat-good", "stat-moderate", "stat-warn", "stat-hot", "stat-critical"};
    for (auto name : classes) {
        gtk_style_context_remove_class(ctx, name);
    }
    if (!cls.empty()) {
        gtk_style_context_add_class(ctx, cls.c_str());
    }
}
