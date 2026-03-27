#!/bin/bash

# Current GPU status for the menu label
GPU_STAT=$(envycontrol --query | awk '{print $NF}')

# Short names as requested
options="  Off\n󰑐  Restart\n󰍃  logout\n󰢮  NV\n󰘚  AMD\n󰾲  Now: $GPU_STAT"

chosen="$(echo -e "$options" | wofi --dmenu --conf ~/.config/wofi/config --style ~/.config/wofi/style.css --prompt "")"

case $chosen in
    *"Off") systemctl poweroff ;;
    *"Restart") systemctl reboot ;;
    *"logout") hyprctl dispatch exit ;;
    *"NV") sudo envycontrol -s nvidia ;;
    *"AMD") sudo envycontrol -s integrated ;;
esac
