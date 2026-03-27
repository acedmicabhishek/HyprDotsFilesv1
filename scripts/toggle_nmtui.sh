#!/bin/bash

# A script to toggle nmtui in a floating kitty window

CLASS="floating_terminal"

# Use hyprctl to check if a window with this class is currently open
if hyprctl clients | grep -q "class: $CLASS"; then
    # If it is, kill the kitty process that belongs to that window
    # Fining the PID might be safer but pkill -f should work now that we know what to match
    pkill -f "kitty --class $CLASS"
else
    # Launch it
    kitty --class "$CLASS" -e nmtui
fi
