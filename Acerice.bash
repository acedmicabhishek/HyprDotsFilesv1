#!/bin/bash

# Acerice Installer Script
# Installs Hyprland, Wofi, SWWW, and copies configurations.

echo "Welcome to the Acerice Installer!"

# 1. Install Dependencies
echo "[*] Updating system and installing dependencies..."
if command -v pacman &> /dev/null; then
    sudo pacman -Syu --noconfirm
    sudo pacman -S --noconfirm hyprland wofi kitty grim slurp swappy thunar network-manager-applet ttf-jetbrains-mono-nerd swayosd brightnessctl wireplumber libnotify
else
    echo "[!] pacman not found. Please install dependencies manually."
fi

# Check for AUR helper (yay or paru) for swww
echo "[*] Installing swww (Wallpaper Daemon)..."
if command -v yay &> /dev/null; then
    yay -S --noconfirm swww
elif command -v paru &> /dev/null; then
    paru -S --noconfirm swww
else
    echo "[!] No AUR helper found (yay/paru). Skipping swww install. Please install 'swww' manually."
fi

# 2. Backup Existing Configs
echo "[*] Backing up existing configurations..."
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

if [ -d "$HOME/.config/hypr" ]; then
    mv "$HOME/.config/hypr" "$HOME/.config/hypr.bak_$TIMESTAMP"
    echo "    Backed up ~/.config/hypr to ~/.config/hypr.bak_$TIMESTAMP"
fi

if [ -d "$HOME/.config/wofi" ]; then
    mv "$HOME/.config/wofi" "$HOME/.config/wofi.bak_$TIMESTAMP"
    echo "    Backed up ~/.config/wofi to ~/.config/wofi.bak_$TIMESTAMP"
fi

# 3. Copy New Configs
echo "[*] Installing new configurations..."

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Copy Hyprland config (Assuming script is inside the 'hypr' folder being distributed)
# If distributing, the user should have a folder structure like:
# /dotfiles
#   /hypr
#   /wofi
#   Acerice.bash
#
# But currently the script IS inside ~/.config/hypr.
# Let's assume the user distributes the PARENT directory containing hypr and wofi.
# OR, they copy this script and the folders.

# We will try to copy 'hypr' and 'wofi' from the script's location.
# If the script is run from inside the 'hypr' folder:
if [ -f "$SCRIPT_DIR/hyprland.conf" ]; then
    mkdir -p "$HOME/.config/hypr"
    cp -r "$SCRIPT_DIR/"* "$HOME/.config/hypr/"
    echo "    Installed Hyprland config."
else
    echo "[!] Could not find hyprland.conf in script directory. skipping hypr install."
fi

# Copy Wofi config
if [ -d "$SCRIPT_DIR/../wofi" ]; then
    mkdir -p "$HOME/.config/wofi"
    cp -r "$SCRIPT_DIR/../wofi/"* "$HOME/.config/wofi/"
    echo "    Installed Wofi config."
elif [ -d "$SCRIPT_DIR/wofi" ]; then
    mkdir -p "$HOME/.config/wofi"
    cp -r "$SCRIPT_DIR/wofi/"* "$HOME/.config/wofi/"
    echo "    Installed Wofi config (from subdirectory)."
else 
    echo "[!] Could not find wofi config folder. Please copy ~/.config/wofi manually."
fi

# Fix Wofi CSS import path for the new user (CSS doesn't support ~ or $HOME)
if [ -f "$HOME/.config/wofi/style.css" ]; then
    sed -i "s|@import.*;|@import \"$HOME/.config/wofi/colors.css\";|g" "$HOME/.config/wofi/style.css"
    echo "    Updated Wofi CSS import path."
fi

# 4. Make scripts executable
chmod +x "$HOME/.config/hypr/scripts/"*.sh 2>/dev/null

echo "Installation complete! Please restart Hyprland."
