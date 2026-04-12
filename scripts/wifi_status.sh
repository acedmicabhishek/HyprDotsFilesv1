#!/usr/bin/env bash

iface=$(nmcli -t -f DEVICE,TYPE dev status | awk -F: '$2=="wifi" {print $1; exit}')
if [ -z "$iface" ]; then
    echo "󰖩 Wi-Fi N/A"
    exit 0
fi

wifi_state=$(nmcli -t -f WIFI g 2>/dev/null | tr -d '\r')
if [ "$wifi_state" = "enabled" ]; then
    icon="󰤨"
    ssid=$(nmcli -t -f active,ssid dev wifi 2>/dev/null | awk -F: '$1=="yes" {print $2; exit}')
else
    icon="󰤭"
    ssid="off"
fi

rx_file="/sys/class/net/$iface/statistics/rx_bytes"
tx_file="/sys/class/net/$iface/statistics/tx_bytes"
if [ ! -r "$rx_file" ] || [ ! -r "$tx_file" ]; then
    echo "$icon ${ssid:-unknown}"
    exit 0
fi

rx=$(cat "$rx_file")
tx=$(cat "$tx_file")
now=$(date +%s)
state_file="/tmp/wifi_speed_${iface}.state"

if [ -f "$state_file" ]; then
    read -r prev_rx prev_tx prev_time < "$state_file"
    dt=$((now - prev_time))
    if [ "$dt" -gt 0 ]; then
        dr=$(( (rx - prev_rx) / dt ))
        dt_rate=$(( (tx - prev_tx) / dt ))
    fi
fi

printf "%s %s" "$icon" "${ssid:-unknown}"
if [ -n "$dr" ] && [ -n "$dt_rate" ]; then
    printf " ↓%s ↑%s" "$(numfmt --to=iec --format=%.1f "$dr")" "$(numfmt --to=iec --format=%.1f "$dt_rate")"
fi
printf "\n"

printf "%s %s %s\n" "$rx" "$tx" "$now" > "$state_file"
