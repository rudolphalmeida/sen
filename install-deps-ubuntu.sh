set -x

sudo apt install autoconf automake libtool pkg-config

echo "Installing SDL3 dependencies"
sudo apt install libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev
sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules

echo "Installing libsystemd dependencies"
sudo apt install python3-jinja2
