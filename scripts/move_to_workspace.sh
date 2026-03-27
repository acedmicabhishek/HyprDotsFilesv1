#!/bin/bash

# A script to move current window to workspace and show an OSD via swayosd

WS="$1"

# Move active window to workspace
hyprctl dispatch movetoworkspace "$WS"

# Show OSD
swayosd-client --custom-message "Moved to Workspace $WS"
