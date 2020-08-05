#!/usr/bin/env python3
import gi
import os
import threading
gi.require_version("Gtk", "3.0")
from gi.repository import GLib, Gtk

ATTACH_PIPE_PATH = "/tmp/pibox_attach_pipe"
ALERT_PIPE_PATH = "/tmp/pibox_alert_pipe"


def attach_notify(msg):
    """ Notifies the main program that the attach button was pressed """
    with open(ATTACH_PIPE_PATH, "w") as f_obj:
        f_obj.write(msg)


class GridWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Pibox UI")

        grid = Gtk.Grid()
        self.add(grid)

        self.attach_button = Gtk.Button(label="Attach HID devices")
        self.attach_button.connect("clicked", self.on_attach_clicked)

        self.alert_label = Gtk.Label(label="")

        grid.attach(self.alert_label, 0, 0, 10, 10)
        grid.attach(self.attach_button, 0, 15, 10, 10)

    def on_attach_clicked(self, widget):
        attach_notify("Reattach")


def main():

    grid_win = GridWindow()
    grid_win.connect("destroy", Gtk.main_quit)

    def update_detection_status(msg):
        grid_win.alert_label.set_label(msg)

    def detection_listener():
        while True:
            with open(ALERT_PIPE_PATH, "r") as f_obj:
                msg = f_obj.read().strip()
                if msg == "Attack Detected":
                    print("Attack Detected")

    grid_win.show_all()
    alert_thread = threading.Thread(target=detection_listener)
    alert_thread.start()


if __name__ == '__main__':
    main()
    Gtk.main()
