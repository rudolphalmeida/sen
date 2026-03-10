set -x

sudo dnf install autoconf autoconf-archive automake libtool perl-open perl-FindBin

# SDL3 deps.
sudo dnf install gcc git-core make cmake \
alsa-lib-devel fribidi-devel pulseaudio-libs-devel pipewire-devel \
libX11-devel libXext-devel libXrandr-devel libXcursor-devel libXfixes-devel \
libXi-devel libXScrnSaver-devel libXtst-devel dbus-devel ibus-devel \
systemd-devel mesa-libGL-devel libxkbcommon-devel mesa-libGLES-devel \
mesa-libEGL-devel vulkan-devel wayland-devel wayland-protocols-devel \
libdrm-devel mesa-libgbm-devel libusb1-devel libdecor-devel \
pipewire-jack-audio-connection-kit-devel libthai-devel zlib-ng-compat-static
