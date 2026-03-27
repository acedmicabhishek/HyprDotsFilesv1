#!/bin/bash
WALL_DIR="$HOME/Pictures/Wallpapers"
INDEX_FILE="$HOME/.cache/.wallpaper_index"
LOG_FILE="$HOME/.cache/wallpaper_log.txt"

echo "--- Script started at $(date) ---" >> "$LOG_FILE"

mapfile -t WALLPAPERS < <(find "$WALL_DIR" -type f \( -name "*.jpg" -o -name "*.png" -o -name "*.webp" \) | sort)

INDEX=$(cat "$INDEX_FILE" 2>/dev/null || echo 0)

if [ "$INDEX" -ge "${#WALLPAPERS[@]}" ]; then
    INDEX=0
fi

SELECTED_WALL="${WALLPAPERS[$INDEX]}"
NEXT_INDEX=$(( (INDEX + 1) % ${#WALLPAPERS[@]} ))
echo "$NEXT_INDEX" > "$INDEX_FILE"

echo "Selected Wallpaper: $SELECTED_WALL" >> "$LOG_FILE"

swww img "$SELECTED_WALL" --transition-type wave --transition-fps 144 --transition-step 90

# --- Color Sync Logic ---
# Extract dominant color using ImageMagick
if command -v magick &> /dev/null; then
    echo "ImageMagick found. Extracting colors..." >> "$LOG_FILE"
    
    # Get hex color (RRGGBB). 
    HEX_COLOR=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -format "%[hex:u]" info: | cut -c 1-6)
    echo "Extracted Accent: $HEX_COLOR" >> "$LOG_FILE"
    
    # Hyprland format: rgba(RRGGBBff)
    echo "\$accent = rgba(${HEX_COLOR}ff)" > /home/light/.config/hypr/colors/color_var.conf
    
    # Wofi format: #RRGGBB
    # Generate a darker version for background: darken by 80% (modulate brightness 20)
    DARK_HEX=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -modulate 100,100,20 -format "%[hex:u]" info: | cut -c 1-6)
    echo "Extracted BG: $DARK_HEX" >> "$LOG_FILE"
    
    echo "@define-color accent #${HEX_COLOR};" > /home/light/.config/wofi/colors.css
    echo "@define-color bg_color #${DARK_HEX}cc;" >> /home/light/.config/wofi/colors.css # 80 = 50% alpha

    # Kitty format
    echo "active_tab_background #${HEX_COLOR}" > /home/light/.config/kitty/colors.conf
    echo "active_border_color #${HEX_COLOR}" >> /home/light/.config/kitty/colors.conf
    echo "cursor #${HEX_COLOR}" >> /home/light/.config/kitty/colors.conf
    # Reload Kitty instances

    pkill -USR1 kitty

    # Dolphin / KDE Globals (Needs RGB format: R,G,B)
    # Extract RGB values
    RGB_COLOR=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -format "%[fx:int(255*r)],%[fx:int(255*g)],%[fx:int(255*b)]" info:)
    
    # Update kdeglobals safely using sed
    KDE_GLOBALS="$HOME/.config/kdeglobals"
    if [ -f "$KDE_GLOBALS" ]; then
        sed -i "s/^BackgroundNormal=.*/BackgroundNormal=$RGB_COLOR/" "$KDE_GLOBALS"
        sed -i "s/^BackgroundAlternate=.*/BackgroundAlternate=$RGB_COLOR/" "$KDE_GLOBALS"
        sed -i "s/^DecorationFocus=.*/DecorationFocus=$RGB_COLOR/" "$KDE_GLOBALS"
        # Newer KDE accent color support
        sed -i "s/^AccentColor=.*/AccentColor=$RGB_COLOR/" "$KDE_GLOBALS"
    fi

    # GTK 3 (Thunar)
    echo "@define-color accent_color #${HEX_COLOR};" > /home/light/.config/gtk-3.0/colors.css
    echo "@define-color accent_bg_color #${HEX_COLOR};" >> /home/light/.config/gtk-3.0/colors.css
    echo "@define-color theme_selected_bg_color #${HEX_COLOR};" >> /home/light/.config/gtk-3.0/colors.css
    echo "@define-color theme_selected_fg_color #11111b;" >> /home/light/.config/gtk-3.0/colors.css

    hyprctl reload
    
    notify-send "Wallpaper & Colors Updated" "Accent: #$HEX_COLOR"
else
    echo "ImageMagick (magick) not found!" >> "$LOG_FILE"
    notify-send "Error" "ImageMagick not found. Colors not synced."
fi
