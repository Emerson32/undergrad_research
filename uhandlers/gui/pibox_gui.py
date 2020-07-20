#!/usr/bin/env python3
import gi
import os
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk


class GridWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Pibox UI")

        grid = Gtk.Grid()
        self.add(grid)

        self.attach_button = Gtk.Button(label="Attach HID devices")
        self.attach_button.connect("clicked", self.on_attach_clicked)
        grid.add(self.attach_button)

    def on_attach_clicked(self, widget):
        print("Hello, world")


grid_win = GridWindow()
grid_win.connect("destroy", Gtk.main_quit)
grid_win.show_all()
Gtk.main()
