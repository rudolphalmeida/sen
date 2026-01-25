set -x

sudo dnf install autoconf autoconf-archive automake libtool perl-open perl-FindBin

# OpenGL and windowing reqs.
sudo dnf install libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
sudo dnf install wayland-devel libxkbcommon-devel wayland-protocols-devel extra-cmake-modules
sudo dnf install mesa-libGL-devel
