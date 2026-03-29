#!/bin/bash
STATUS=$(AAC --status)

if [ "$1" == "power" ]; then
    if echo "$STATUS" | grep -qi "turbo"; then
        sudo AAC -P silent
    elif echo "$STATUS" | grep -qi "silent"; then
        sudo AAC -P balanced
    else
        sudo AAC -P turbo
    fi
elif [ "$1" == "gpu" ]; then
    if echo "$STATUS" | grep -qi "eco"; then
        sudo AAC -G hybrid
        notify-send "GPU Mode" "Set to Hybrid. Reboot required!"
    else
        sudo AAC -G eco
        notify-send "GPU Mode" "Set to Eco. Reboot required!"
    fi
fi
