#!/bin/bash
# 1. Grab the color in HEX format using hyprpicker
PICKED_COLOR=$(hyprpicker -a -f hex)

# 2. Update your Hyprland variable file
# We remove the '#' for the Hyprland rgba format (e.g., #ffffff -> ffffff)
HEX_STRIP=${PICKED_COLOR:1}
echo "\$accent = rgba(${HEX_STRIP}ff)" > /home/light/.config/hypr/colors/color_var.conf

# 3. Update Wofi's dedicated color file
# Wofi needs standard CSS format
echo "@define-color accent $PICKED_COLOR;" > /home/light/.config/wofi/colors.css

# 4. Reload Hyprland to apply the new border colors
hyprctl reload