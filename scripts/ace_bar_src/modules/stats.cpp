#include "stats.hpp"
#include "../globals.hpp"
#include "../utils.hpp"
#include <fstream>
#include <sstream>

void update_stats() {
  try {
    // CPU
    std::ifstream stat_file("/proc/stat");
    std::string line;
    for (int i = 0; i <= App.cpu_count; ++i) {
      if (!std::getline(stat_file, line)) break;
      std::istringstream iss(line);
      std::string cpu_hdr;
      long u=0, n=0, s=0, id=0, iowait=0, irq=0, softirq=0, steal=0, guest=0, guest_nice=0;
      iss >> cpu_hdr >> u >> n >> s >> id >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
      
      if (cpu_hdr.find("cpu") != 0) break;

      long idle = id + iowait;
      long total_time = u + n + s + idle + irq + softirq + steal + guest + guest_nice;
      std::vector<long> curr = {total_time, idle};

      if (!App.last_cpu_times[i].empty()) {
        long d_total = curr[0] - App.last_cpu_times[i][0];
        long d_idle = curr[1] - App.last_cpu_times[i][1];
        if (d_total > 0) {
          float usage = (float)(d_total - d_idle) / d_total;
          if (i == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "󰍛 %.1f%%", usage * 100);
            safe_set_label(App.modules["cpu"], buf);
          }
        }
      }
      App.last_cpu_times[i] = curr;
    }

    // Memory
    std::ifstream mem_file("/proc/meminfo");
    long total_kb = 0, avail_kb = 0;
    while (std::getline(mem_file, line)) {
      if (line.find("MemTotal:") == 0) sscanf(line.c_str(), "MemTotal: %ld", &total_kb);
      if (line.find("MemAvailable:") == 0) sscanf(line.c_str(), "MemAvailable: %ld", &avail_kb);
    }
    if (total_kb > 0) {
        long used_kb = total_kb - avail_kb;
        float used_gb = (float)used_kb / 1024 / 1024;
        float tot_gb = (float)total_kb / 1024 / 1024;
        char buf[64];
        snprintf(buf, sizeof(buf), "󰘚 %.1fG", used_gb);
        safe_set_label(App.modules["mem"], buf);
    }
  } catch (...) {}
}
