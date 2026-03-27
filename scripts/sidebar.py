#!/usr/bin/env python3
import gi
import subprocess
import os

gi.require_version('Gtk', '4.0')
gi.require_version('Gtk4LayerShell', '1.0')
from gi.repository import Gtk, Gdk, GLib, Gio, GObject, Gtk4LayerShell as LayerShell

WIDTH = 300
HIDE_MARGIN = -WIDTH + 2
REVEAL_MARGIN = 0

class Sidebar(Gtk.ApplicationWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # Initialize Layer Shell
        LayerShell.init_for_window(self)
        LayerShell.set_layer(self, LayerShell.Layer.TOP)
        LayerShell.set_anchor(self, LayerShell.Edge.RIGHT, True)
        LayerShell.set_anchor(self, LayerShell.Edge.TOP, True)
        LayerShell.set_anchor(self, LayerShell.Edge.BOTTOM, True)
        LayerShell.set_margin(self, LayerShell.Edge.RIGHT, HIDE_MARGIN)

        # Styling
        css_provider = Gtk.CssProvider()
        css_data = """
            window {
                background: rgba(20, 20, 30, 0.75);
                border-left: 1px solid rgba(255, 255, 255, 0.15);
                border-radius: 20px 0 0 20px;
            }
            .container {
                padding: 30px;
            }
            .title {
                font-size: 26px;
                font-weight: 800;
                color: #ffffff;
                margin-bottom: 30px;
            }
            .stat-card {
                background: rgba(255, 255, 255, 0.05);
                padding: 15px;
                border-radius: 12px;
                margin-bottom: 15px;
            }
            .stat-label {
                color: #888888;
                font-size: 12px;
                font-weight: 700;
                margin-bottom: 5px;
            }
            .stat-value {
                color: #ffffff;
                font-size: 18px;
                font-weight: 600;
            }
        """
        css_provider.load_from_data(css_data.encode())
        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        # UI
        self.container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, css_classes=['container'], width_request=WIDTH)
        self.set_child(self.container)

        title = Gtk.Label(label='SYSTEM DATA', css_classes=['title'], xalign=0)
        self.container.append(title)

        # Stats Sections
        self.mode_val = self._add_stat('ASUS PERFORMANCE', 'Loading...')
        self.bat_val = self._add_stat('BATTERY STATUS', 'Loading...')
        self.temp_val = self._add_stat('RYZEN TEMP', 'Loading...')

        # Hover logic
        self.is_revealed = False
        motion = Gtk.EventControllerMotion()
        motion.connect("enter", self._on_enter)
        motion.connect("leave", self._on_leave)
        self.add_controller(motion)

        # Update loop
        GLib.timeout_add_seconds(2, self._update_stats)
        self._update_stats()

    def _add_stat(self, label_text, value_text):
        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, css_classes=['stat-card'])
        lbl = Gtk.Label(label=label_text, css_classes=['stat-label'], xalign=0)
        val = Gtk.Label(label=value_text, css_classes=['stat-value'], xalign=0)
        card.append(lbl)
        card.append(val)
        self.container.append(card)
        return val

    def _on_enter(self, *args):
        if not self.is_revealed:
            self.is_revealed = True
            LayerShell.set_margin(self, LayerShell.Edge.RIGHT, REVEAL_MARGIN)

    def _on_leave(self, *args):
        # We only hide if the mouse is far enough left (out of the panel)
        # But for now, simple toggle
        if self.is_revealed:
            self.is_revealed = False
            LayerShell.set_margin(self, LayerShell.Edge.RIGHT, HIDE_MARGIN)

    def _update_stats(self):
        try:
            res = subprocess.check_output(['/home/light/.config/hypr/scripts/sys_data.sh'], encoding='utf-8')
            for line in res.split('\n'):
                if 'ASUS Mode:' in line:
                    self.mode_val.set_label(line.split(':', 1)[1].strip())
                elif 'Battery:' in line:
                    self.bat_val.set_label(line.split(':', 1)[1].strip())
                elif 'Ryzen Max Temp:' in line:
                    self.temp_val.set_label(line.split(':', 1)[1].strip())
        except Exception as e:
            print(f"Error updating stats: {e}")
        return True

class SidebarApp(Gtk.Application):
    def __init__(self):
        super().__init__(application_id='com.acerice.sidebar')

    def do_activate(self):
        win = Sidebar(application=self)
        win.present()

if __name__ == "__main__":
    app = SidebarApp()
    app.run(None)
