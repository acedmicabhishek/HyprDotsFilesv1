#include "globals.hpp"
#include "ui.hpp"
#include "modules/stats.hpp"
#include "modules/workspaces.hpp"
#include "modules/system.hpp"
#include "modules/weather.hpp"
#include <fcntl.h>
#include <glib-unix.h>
#include <unistd.h>
#include <fstream>
#include <thread>

AppState App;

static gboolean on_timer_1s(gpointer data) {
    update_time();
    update_volume();
    update_net();
    update_wifi();
    return TRUE;
}

static gboolean on_timer_2s(gpointer data) {
    update_stats();
    update_system();
    update_workspaces();
    return TRUE;
}

static gboolean on_sigusr1(gpointer data) {
  toggle_visibility();
  return TRUE;
}

static void on_activate(GtkApplication *app, gpointer user_data) {
  std::ifstream check_lock(LOCK_FILE);
  if (check_lock.is_open()) {
    pid_t old_pid;
    check_lock >> old_pid;
    check_lock.close();
    if (kill(old_pid, 0) == 0) exit(0);
    unlink(LOCK_FILE);
  }
  int fd = open(LOCK_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  char pid_s[16];
  snprintf(pid_s, 16, "%d", getpid());
  write(fd, pid_s, strlen(pid_s));
  close(fd);

  setup_ui(app);
  g_unix_signal_add(SIGUSR1, on_sigusr1, NULL);

  g_timeout_add_seconds(1, on_timer_1s, NULL);
  g_timeout_add_seconds(2, on_timer_2s, NULL);
  std::thread(weather_loop).detach();
}

int main(int argc, char **argv) {
  App.cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  App.last_cpu_times.resize(App.cpu_count + 1);
  GtkApplication *app = gtk_application_new("com.ace.bar", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  unlink(LOCK_FILE);
  return status;
}
