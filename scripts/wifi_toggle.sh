#!/usr/bin/env bash

wifi_state=$(nmcli -t -f WIFI g 2>/dev/null | tr -d '\r')
if [ "$wifi_state" = "enabled" ]; then
    nmcli radio wifi off >/dev/null 2>&1
else
    nmcli radio wifi on >/dev/null 2>&1
fi
