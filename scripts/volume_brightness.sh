#!/bin/bash

# Simple OSD widget for Volume and Brightness using notify-send
# It uses the synchronous hint to update an existing notification bubble with a progress bar

function get_volume {
    wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print int($2 * 100)}'
}

function get_mute {
    wpctl get-volume @DEFAULT_AUDIO_SINK@ | grep -q MUTED && echo "yes" || echo "no"
}

function get_brightness {
    brightnessctl -m | awk -F, '{print substr($4, 1, length($4)-1)}'
}

case $1 in
    volume_up)
        wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%+
        vol=$(get_volume)
        notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -h int:value:$vol -i audio-volume-high "Volume: ${vol}%"
        ;;
    volume_down)
        wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-
        vol=$(get_volume)
        notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -h int:value:$vol -i audio-volume-medium "Volume: ${vol}%"
        ;;
    volume_mute)
        wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle
        if [ "$(get_mute)" == "yes" ]; then
            notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -i audio-volume-muted "Volume Muted"
        else
            vol=$(get_volume)
            notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -h int:value:$vol -i audio-volume-high "Volume Unmuted"
        fi
        ;;
    brightness_up)
        brightnessctl set 5%+
        bri=$(get_brightness)
        notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -h int:value:$bri -i display-brightness "Brightness: ${bri}%"
        ;;
    brightness_down)
        brightnessctl set 5%-
        bri=$(get_brightness)
        notify-send -t 1500 -h string:x-canonical-private-synchronous:sys-notify -h int:value:$bri -i display-brightness "Brightness: ${bri}%"
        ;;
esac
