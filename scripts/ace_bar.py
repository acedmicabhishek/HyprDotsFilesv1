import gi
import os
import subprocess
import json
import signal
import sys
import threading

gi.require_version('Gtk', '4.0')
gi.require_version('Gtk4LayerShell', '1.0')
from gi.repository import Gtk, Gdk, GLib, Gtk4LayerShell

CSS_FILE = os.path.expanduser("~/.config/hypr/scripts/ace_bar.css")
LOCK_FILE = "/tmp/ace_bar.lock"

class AceBar(Gtk.Application):
    def __init__(self):
        super().__init__(application_id="com.ace.bar")
        self.is_visible = True
        self.net_last_recv = 0
        self.net_last_sent = 0
        self.ws_buttons = {}
        self.gpu_loading = False

    def do_activate(self):
        # Lock check
        if os.path.exists(LOCK_FILE):
            print("AceBar already running. Exit.")
            sys.exit(0)
        with open(LOCK_FILE, 'w') as f:
            f.write(str(os.getpid()))

        self.win = Gtk.ApplicationWindow(application=self)
        self.win.connect("destroy", lambda w: os.remove(LOCK_FILE))
        
        # Setup Layer Shell
        Gtk4LayerShell.init_for_window(self.win)
        Gtk4LayerShell.set_layer(self.win, Gtk4LayerShell.Layer.TOP)
        Gtk4LayerShell.set_margin(self.win, Gtk4LayerShell.Edge.TOP, 0)
        Gtk4LayerShell.set_anchor(self.win, Gtk4LayerShell.Edge.TOP, True)
        Gtk4LayerShell.set_anchor(self.win, Gtk4LayerShell.Edge.LEFT, True)
        Gtk4LayerShell.set_anchor(self.win, Gtk4LayerShell.Edge.RIGHT, True)
        Gtk4LayerShell.set_exclusive_zone(self.win, 30)
        
        # Main Layout
        self.main_box = Gtk.CenterBox()
        self.main_box.add_css_class("bar-container")
        
        # Left: Workspaces
        self.workspace_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
        self.workspace_box.add_css_class("island")
        self.workspace_box.add_css_class("left")
        self.main_box.set_start_widget(self.workspace_box)
        
        # Create persistent workspace buttons
        for i in range(1, 11):
            btn = Gtk.Button(label=str(i))
            btn.add_css_class("ws-btn")
            btn.connect("clicked", lambda b, num=i: subprocess.run(["hyprctl", "dispatch", "workspace", str(num)]))
            self.workspace_box.append(btn)
            self.ws_buttons[i] = btn
        
        # Center: Clock & Title & Media
        self.center_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=15)
        self.center_box.add_css_class("island")
        self.center_box.add_css_class("center")
        
        self.media_label = Gtk.Label(label="")
        self.media_label.add_css_class("media-text")
        self.center_box.append(self.media_label)
        
        # Click to play/pause media
        media_gesture = Gtk.GestureClick()
        media_gesture.connect("released", self.on_media_click)
        self.media_label.add_controller(media_gesture)

        self.title_label = Gtk.Label(label="Desktop")
        self.title_label.add_css_class("window-title")
        self.center_box.append(self.title_label)
        
        self.clock_label = Gtk.Label(label="00:00")
        self.center_box.append(self.clock_label)
        
        self.main_box.set_center_widget(self.center_box)
        
        # Right: Hardware Modules
        self.hardware_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.hardware_box.add_css_class("island")
        self.hardware_box.add_css_class("right")
        self.main_box.set_end_widget(self.hardware_box)
        
        self.win.set_child(self.main_box)
        self.win.present()
        
        # Modules setup
        self.modules = {}
        for mod in ["net", "gpu", "power", "vol", "bright", "cpu", "mem", "temp", "bat"]:
            lbl = Gtk.Label(label="...")
            lbl.add_css_class(f"mod-{mod}")
            self.hardware_box.append(lbl)
            self.modules[mod] = lbl
            
            if mod in ["power", "gpu", "vol", "bright"]:
                gesture = Gtk.GestureClick()
                if mod == "power": gesture.connect("released", self.on_power_click)
                elif mod == "gpu": gesture.connect("released", self.on_gpu_click)
                elif mod == "vol": gesture.connect("released", self.on_vol_click)
                elif mod == "bright": gesture.connect("released", self.on_bright_click)
                lbl.add_controller(gesture)

        self.load_css()
        
        GLib.timeout_add_seconds(1, self.update_clock)
        GLib.timeout_add_seconds(2, self.update_hardware)
        GLib.timeout_add(1000, self.update_network)
        GLib.timeout_add(500, self.update_workspaces)
        GLib.timeout_add(1000, self.update_title)
        GLib.timeout_add_seconds(1, self.update_vol)
        GLib.timeout_add_seconds(2, self.update_media)
        GLib.timeout_add_seconds(2, self.update_stats)
        GLib.timeout_add_seconds(2, self.update_brightness)

        # Signal handling for toggle (SIGUSR1)
        GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGUSR1, self.toggle_visibility)

    def load_css(self):
        provider = Gtk.CssProvider()
        provider.load_from_path(CSS_FILE)
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def toggle_visibility(self):
        if self.is_visible:
            self.main_box.add_css_class("hidden-bar")
            self.drawer_center.hide()
            self.drawer_right.hide()
            Gtk4LayerShell.set_exclusive_zone(self.win, 0)
        else:
            self.main_box.remove_css_class("hidden-bar")
            self.drawer_center.present()
            self.drawer_right.present()
            Gtk4LayerShell.set_exclusive_zone(self.win, 30)
        self.is_visible = not self.is_visible
        return True

    def update_clock(self):
        raw_date = subprocess.check_output(["date", "+%A, %d %B"]).decode().strip()
        raw_time = subprocess.check_output(["date", "+%H:%M:%S"]).decode().strip()
        self.clock_label.set_label(f" {raw_time[:-3]}  󰃭 {raw_date.split(',')[1].strip()}")
        self.big_clock.set_label(raw_time)
        self.big_date.set_label(raw_date)
        return True

    def update_title(self):
        try:
            active = subprocess.check_output(["hyprctl", "activewindow", "-j"]).decode()
            title = json.loads(active).get("title", "Desktop")
            full_title = title
            if len(title) > 30: title = title[:27] + "..."
            self.title_label.set_label(f"聚焦: {title}")
            self.active_win_drawer.set_label(f"Focused: {full_title[:50]}")
        except: 
            self.title_label.set_label("Desktop")
            self.active_win_drawer.set_label("Focused: Desktop")
        return True

    def update_media(self):
        try:
            status = subprocess.check_output(["playerctl", "status"]).decode().strip()
            if status:
                artist = subprocess.check_output(["playerctl", "metadata", "artist"]).decode().strip()
                title = subprocess.check_output(["playerctl", "metadata", "title"]).decode().strip()
                icon = "󰐊" if status == "Playing" else "󰏤"
                m_text = f"{icon} {artist} - {title}"
                if len(m_text) > 35: m_text = m_text[:32] + "..."
                self.media_label.set_label(m_text)
            else: self.media_label.set_label("")
        except: self.media_label.set_label("")
        return True

    def on_media_click(self, gesture, n_press, x, y):
        subprocess.run(["playerctl", "play-pause"])
        self.update_media()

    def update_vol(self):
        try:
            vol_out = subprocess.check_output(["wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@"]).decode()
            vol = int(float(vol_out.split()[1]) * 100)
            muted = "MUTED" in vol_out
            icon = "󰝟" if muted or vol == 0 else "󰕾"
            self.modules["vol"].set_label(f"{icon} {vol}%")
        except: pass
        return True

    def on_vol_click(self, gesture, n_press, x, y):
        # Mute toggle
        subprocess.run(["wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle"])
        self.update_vol()

    def update_brightness(self):
        try:
            out = subprocess.check_output(["brightnessctl", "g"]).decode().strip()
            max_b = subprocess.check_output(["brightnessctl", "m"]).decode().strip()
            level = int((int(out) / int(max_b)) * 100)
            self.modules["bright"].set_label(f"󰃠 {level}%")
        except: pass
        return True

    def on_bright_click(self, gesture, n_press, x, y):
        # Cycle brightness: 25 -> 50 -> 75 -> 100 -> 25
        try:
            out = subprocess.check_output(["brightnessctl", "g"]).decode().strip()
            max_b = subprocess.check_output(["brightnessctl", "m"]).decode().strip()
            level = int((int(out) / int(max_b)) * 100)
            new_level = 25
            if level < 25: new_level = 25
            elif level < 50: new_level = 50
            elif level < 75: new_level = 75
            elif level < 100: new_level = 100
            else: new_level = 25
            subprocess.run(["brightnessctl", "s", f"{new_level}%"])
            self.update_brightness()
        except: pass

    def update_stats(self):
        # CPU Detailed
        try:
            with open("/proc/stat") as f:
                lines = f.readlines()
            
            # Per core update
            # Line 0 is 'cpu', Line 1..N are 'cpu0'..'cpuN'
            for idx in range(self.cpu_count + 1):
                line = lines[idx].split()
                if not line[0].startswith("cpu"): continue
                
                curr_times = [int(x) for x in line[1:5]] # user, nice, system, idle
                if self.last_cpu_times[idx]:
                    diff = [curr_times[i] - self.last_cpu_times[idx][i] for i in range(4)]
                    total = sum(diff)
                    if total > 0:
                        usage = (total - diff[3]) / total
                        if idx == 0: # Global
                            self.modules["cpu"].set_label(f"󰍛 {usage*100:.1f}%")
                        else: # Per core
                            self.cpu_bars[idx-1].set_fraction(usage)
                self.last_cpu_times[idx] = curr_times
        except: pass
        
        # Memory Usage
        try:
            with open("/proc/meminfo") as f:
                lines = f.readlines()
            mem_total = int(lines[0].split()[1])
            mem_avail = int(lines[2].split()[1])
            used_kb = mem_total - mem_avail
            used_gb = used_kb / 1024 / 1024
            total_gb = mem_total / 1024 / 1024
            perc = used_kb / mem_total
            
            self.modules["mem"].set_label(f"󰘚 {used_gb:.1f}G")
            self.ram_bar.set_fraction(perc)
            self.ram_detail.set_label(f"{used_gb:.1f}G / {total_gb:.1f}G")
        except: pass
        return True

    def update_network(self):
        try:
            with open("/proc/net/dev") as f:
                lines = f.readlines()[2:]
            
            total_recv = 0
            total_sent = 0
            for line in lines:
                parts = line.split()
                if parts[0] == "lo:": continue
                total_recv += int(parts[1])
                total_sent += int(parts[9])
            
            if self.net_last_recv > 0:
                diff_recv = (total_recv - self.net_last_recv) / 1024
                diff_sent = (total_sent - self.net_last_sent) / 1024
                # Space out data for a cleaner look
                self.modules["net"].set_label(f"󰈀 ↓{diff_recv:.1f} ↑{diff_sent:.1f}")
            
            self.net_last_recv = total_recv
            self.net_last_sent = total_sent
        except: pass
        return True

    def on_power_click(self, gesture, n_press, x, y):
        try:
            # Cycle modes: silent -> balanced -> turbo -> silent
            status = subprocess.check_output(["AAC", "--status"]).decode()
            current_mode = "balanced"
            for line in status.splitlines():
                if "Power Mode" in line:
                    if "silent" in line.lower(): current_mode = "silent"
                    elif "turbo" in line.lower(): current_mode = "turbo"
                    else: current_mode = "balanced"
                    break
            
            new_mode = "balanced"
            if current_mode == "silent": new_mode = "balanced"
            elif current_mode == "balanced": new_mode = "turbo"
            else: new_mode = "silent"
            
            subprocess.run(["sudo", "AAC", "-P", new_mode])
            self.update_hardware()
        except: pass

    def on_gpu_click(self, gesture, n_press, x, y):
        if self.gpu_loading: return
        self.gpu_loading = True
        self.modules["gpu"].set_label("󰚔 切换中...")
        self.modules["gpu"].add_css_class("spin")
        
        def run_switch():
            try:
                status = subprocess.check_output(["AAC", "--status"]).decode()
                current_mode = "hybrid"
                for line in status.splitlines():
                    if "GPU Mode" in line:
                        if "eco" in line.lower(): current_mode = "eco"
                        else: current_mode = "hybrid"
                        break
                
                new_mode = "hybrid" if current_mode == "eco" else "eco"
                subprocess.run(["sudo", "AAC", "-G", new_mode])
            except: pass
            GLib.idle_add(self.on_gpu_finish)

        threading.Thread(target=run_switch, daemon=True).start()

    def on_gpu_finish(self):
        self.gpu_loading = False
        self.modules["gpu"].remove_css_class("spin")
        self.update_hardware()
        return False

    def update_hardware(self):
        if self.gpu_loading: return True
        try:
            status = subprocess.check_output(["AAC", "--status"]).decode()
            p_mode = "..."
            g_mode = "..."
            for line in status.splitlines():
                if "Power Mode" in line:
                    low_line = line.lower()
                    if "silent" in low_line: p_mode = "Silent"
                    elif "balanced" in low_line: p_mode = "Balanced"
                    elif "turbo" in low_line: p_mode = "Turbo"
                elif "GPU Mode" in line:
                    low_line = line.lower()
                    if "eco" in low_line: g_mode = "Eco"
                    elif "hybrid" in low_line: g_mode = "Hybrid"
                    elif "standard" in low_line: g_mode = "Std"
            
            self.modules["power"].set_label(f"󰈐 {p_mode}")
            self.modules["gpu"].set_label(f"󰢮 {g_mode}")
        except: pass

        # Update Temp & Bat (via sys_data.sh)
        for mod in ["temp", "bat"]:
            try:
                val = subprocess.check_output([os.path.expanduser("~/.config/hypr/scripts/sys_data.sh"), mod]).decode().strip()
                icon = self.get_icon(mod)
                label = f"{icon} {val}" if icon else val
                self.modules[mod].set_label(label)
            except: pass
        return True

    def get_icon(self, mod):
        icons = {"temp": ""}
        return icons.get(mod, "")

    def update_workspaces(self):
        try:
            active_ws = subprocess.check_output(["hyprctl", "-j", "activeworkspace"]).decode()
            active_id = json.loads(active_ws)["id"]
            
            for i, btn in self.ws_buttons.items():
                if i == active_id:
                    btn.add_css_class("active")
                else:
                    btn.remove_css_class("active")
        except: pass
        return True

def cleanup(sig, frame):
    if os.path.exists(LOCK_FILE):
        os.remove(LOCK_FILE)
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)
    app = AceBar()
    app.run(None)
