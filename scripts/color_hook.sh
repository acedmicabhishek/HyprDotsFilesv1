#!/bin/bash
# 1. Sample the color
PICKED_COLOR=$(hyprpicker -a -f hex)

# 2. Update Hyprland Variable (for borders/active windows)
echo "\$accent = rgba(${PICKED_COLOR:1}ff)" > ~/.config/hypr/colors.conf

# 3. Update Wofi Variable (for the launcher)
echo "@define-color accent $PICKED_COLOR;" > ~/.config/wofi/colors.css

# 4. Force Hyprland to reload the new colors
hyprctl reload
