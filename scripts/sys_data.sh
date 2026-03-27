#!/bin/bash

# A script to extract system and hardware data for Waybar (Clean Output)

KERNEL_DIR="$HOME/.config/kerneldrive"

# Function to read INI values
read_ini() {
    file=$1
    section=$2
    key=$3
    sed -rn "/^\[$section\]/ { :a; n; s/^$key=(.*)/\1/p; t; ba; }" "$file" 2>/dev/null | tr -d '\r'
}

case $1 in
    mode)
        VAL=$(read_ini "$KERNEL_DIR/asus_armoury/power.ini" "Power" "Mode")
        case $VAL in
            0) echo "Silent" ;;
            1) echo "Balanced" ;;
            2) echo "Turbo" ;;
            *) echo "Unknown" ;;
        esac
        ;;
    temp)
        VAL=$(read_ini "$KERNEL_DIR/ryzen_controller/tuning.ini" "Tuning" "TempLimit")
        [ -n "$VAL" ] && echo "${VAL}°C" || echo "N/A"
        ;;
    bat)
        BAT_PATH=$(ls -d /sys/class/power_supply/BAT* 2>/dev/null | head -n 1)
        if [ -n "$BAT_PATH" ]; then
            PERC=$(cat "$BAT_PATH/capacity")
            STAT=$(cat "$BAT_PATH/status")
            [ "$STAT" = "Charging" ] && echo "󰂄 ${PERC}%" || echo "󰁹 ${PERC}%"
        else
            echo "󰂃 N/A"
        fi
        ;;
    *)
        echo "Use: mode, temp, or bat"
        ;;
esac
