#pragma once
#include <string>
#include <gtk/gtk.h>

std::string exec(const char *cmd);
void safe_set_label(GtkWidget* label, const std::string& text);
