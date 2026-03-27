import gi
import os
import subprocess
import json
import signal
import sys

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
        
        # Center: Clock & Calendar
        self.center_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.center_box.add_css_class("island")
        self.center_box.add_css_class("center")
        
        self.clock_label = Gtk.Label(label="00:00")
        self.center_box.append(self.clock_label)
        
        self.main_box.set_center_widget(self.center_box)
        
        # Right: Hardware & Net & AAC
        self.hardware_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.hardware_box.add_css_class("island")
        self.hardware_box.add_css_class("right")
        self.main_box.set_end_widget(self.hardware_box)
        
        self.win.set_child(self.main_box)
        self.win.present()
        
        # Modules
        self.modules = {}
        # Order: Net, Power (AAC), Temp, Bat
        for mod in ["net", "power", "temp", "bat"]:
            lbl = Gtk.Label(label="...")
            lbl.add_css_class(f"mod-{mod}")
            self.hardware_box.append(lbl)
            self.modules[mod] = lbl
            
            if mod == "power":
                # Make power module clickable
                gesture = Gtk.GestureClick()
                gesture.connect("released", self.on_power_click)
                lbl.add_controller(gesture)

        # Load Styles
        self.load_css()
        
        # Update Loop
        GLib.timeout_add_seconds(1, self.update_clock)
        GLib.timeout_add_seconds(2, self.update_hardware)
        GLib.timeout_add(1000, self.update_network)
        GLib.timeout_add(500, self.update_workspaces)

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
            Gtk4LayerShell.set_exclusive_zone(self.win, 0)
        else:
            self.main_box.remove_css_class("hidden-bar")
            Gtk4LayerShell.set_exclusive_zone(self.win, 30) # Default height
        self.is_visible = not self.is_visible
        return True

    def update_clock(self):
        time_str = subprocess.check_output(["date", "+%H:%M:%S  󰃭 %d/%m"]).decode().strip()
        self.clock_label.set_label(f" {time_str}")
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
                self.modules["net"].set_label(f"󰈀 ↓{diff_recv:.1f} ↑{diff_sent:.1f} KB/s")
            
            self.net_last_recv = total_recv
            self.net_last_sent = total_sent
        except: pass
        return True

    def on_power_click(self, gesture, n_press, x, y):
        try:
            # Cycle modes: silent -> balanced -> turbo -> silent
            status = subprocess.check_output(["AAC", "--status"]).decode()
            current_mode = "silent"
            for line in status.splitlines():
                if "Power Mode" in line:
                    if "balanced" in line.lower(): current_mode = "balanced"
                    elif "turbo" in line.lower(): current_mode = "turbo"
                    else: current_mode = "silent"
                    break
            
            if current_mode == "silent": new_mode = "balanced"
            elif current_mode == "balanced": new_mode = "turbo"
            else: new_mode = "silent"
            
            subprocess.run(["sudo", "AAC", "-P", new_mode])
            self.update_hardware()
        except: pass

    def update_hardware(self):
        # Update Power (AAC)
        try:
            status = subprocess.check_output(["AAC", "--status"]).decode()
            mode = "..."
            for line in status.splitlines():
                if "Power Mode" in line:
                    low_line = line.lower()
                    if "silent" in low_line: mode = "Silent"
                    elif "balanced" in low_line: mode = "Balanced"
                    elif "turbo" in low_line: mode = "Turbo"
                    break
            self.modules["power"].set_label(f"󰈐 {mode}")
        except: pass

        # Update Temp & Bat (via sys_data.sh)
        for mod in ["temp", "bat"]:
            try:
                val = subprocess.check_output([os.path.expanduser("~/.config/hypr/scripts/sys_data.sh"), mod]).decode().strip()
                icon = self.get_icon(mod)
                self.modules[mod].set_label(f"{icon} {val}")
            except: pass
        return True

    def get_icon(self, mod):
        icons = {"temp": "", "bat": "󰁹"}
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
