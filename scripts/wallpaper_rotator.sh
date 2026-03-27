#!/bin/bash
WALLPAPER_DIR="$HOME/Pictures/Wallpapers"
TRANSITION_TYPE="random"
INTERVAL=1800

if ! pgrep -x "swww-daemon" > /dev/null; then
    swww-daemon & disown
    sleep 0.5
fi

while true; do
    WALLPAPER=$(find "$WALLPAPER_DIR" -type f \( -name "*.jpg" -o -name "*.png" -o -name "*.webp" \) | shuf -n 1)
    
    if [ -n "$WALLPAPER" ]; then
        swww img "$WALLPAPER" --transition-type "$TRANSITION_TYPE"
        
        # --- Color Sync Logic (Synced with wallpaper_random.sh) ---
        SELECTED_WALL="$WALLPAPER"
        
        if command -v magick &> /dev/null; then
            # Get hex color (RRGGBB)
            HEX_COLOR=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -format "%[hex:u]" info: | cut -c 1-6)
            
            # Hyprland format
            echo "\$accent = rgba(${HEX_COLOR}ff)" > /home/light/.config/hypr/colors/color_var.conf
            
            # Wofi format
            # Darken by 80% (modulate 20) for background
            DARK_HEX=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -modulate 100,100,20 -format "%[hex:u]" info: | cut -c 1-6)
            
            echo "@define-color accent #${HEX_COLOR};" > /home/light/.config/wofi/colors.css
            echo "@define-color bg_color #${DARK_HEX}80;" >> /home/light/.config/wofi/colors.css 

            # Kitty format
            echo "active_tab_background #${HEX_COLOR}" > /home/light/.config/kitty/colors.conf
            echo "active_border_color #${HEX_COLOR}" >> /home/light/.config/kitty/colors.conf
            echo "cursor #${HEX_COLOR}" >> /home/light/.config/kitty/colors.conf
            # Reload Kitty instances
            pkill -USR1 kitty
        
            # Dolphin / KDE Globals (Needs RGB format: R,G,B)
            RGB_COLOR=$(magick "$SELECTED_WALL" -resize 1x1! -depth 8 -format "%[fx:int(255*r)],%[fx:int(255*g)],%[fx:int(255*b)]" info:)
            
            KDE_GLOBALS="$HOME/.config/kdeglobals"
            if [ -f "$KDE_GLOBALS" ]; then
                sed -i "s/^BackgroundNormal=.*/BackgroundNormal=$RGB_COLOR/" "$KDE_GLOBALS"
                sed -i "s/^BackgroundAlternate=.*/BackgroundAlternate=$RGB_COLOR/" "$KDE_GLOBALS"
                sed -i "s/^DecorationFocus=.*/DecorationFocus=$RGB_COLOR/" "$KDE_GLOBALS"
                sed -i "s/^AccentColor=.*/AccentColor=$RGB_COLOR/" "$KDE_GLOBALS"
            fi
        
            # GTK 3 (Thunar)
            echo "@define-color accent_color #${HEX_COLOR};" > /home/light/.config/gtk-3.0/colors.css
            echo "@define-color accent_bg_color #${HEX_COLOR};" >> /home/light/.config/gtk-3.0/colors.css
            echo "@define-color theme_selected_bg_color #${HEX_COLOR};" >> /home/light/.config/gtk-3.0/colors.css
            echo "@define-color theme_selected_fg_color #11111b;" >> /home/light/.config/gtk-3.0/colors.css
        
            hyprctl reload
        fi
    
    sleep $INTERVAL
done
