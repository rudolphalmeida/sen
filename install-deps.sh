set +x

sudo apt install autoconf automake libtool pkg-config

echo "Installing GLFW dependencies"
sudo apt install libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev

echo "Installing libsystemd dependencies"
sudo apt install python3-jinja2
