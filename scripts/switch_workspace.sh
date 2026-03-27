#!/bin/bash

# A script to switch workspace and show an OSD via swayosd

WS="$1"

# Switch workspace
hyprctl dispatch workspace "$WS"

# Show OSD
swayosd-client --custom-message "Workspace $WS"
