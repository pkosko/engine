#
# Stage for build-engine-base
#
FROM ghcr.io/flutter-tizen/tizen-tools:latest AS build-engine-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y git curl pkg-config ca-certificates xz-utils python python3 libncurses5 && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}


#
# Stage for build-engine-with-efl
#
FROM build-engine-base AS build-engine-with-efl

# Install dependencies for building EFL.
RUN apt-get install -y build-essential check meson ninja-build && \
    apt-get clean
RUN apt-get install -y libssl-dev libsystemd-dev libglib2.0-dev libudev-dev libmount-dev libdbus-1-dev libunwind-dev && \
    apt-get clean
RUN apt-get install -y libjpeg-dev libopenjp2-7-dev libgif-dev libtiff5-dev librsvg2-dev libheif-dev libwebp-dev libraw-dev \
                       libpoppler-dev libpoppler-cpp-dev libspectre-dev libfreetype6-dev libfontconfig1-dev libharfbuzz-dev \
                       libpulse-dev libsndfile1-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                       libibus-1.0-dev libscim-dev libfribidi-dev libinput-dev liblua5.2-dev libluajit-5.1-dev \
                       libx11-dev libxext-dev libxrender-dev libxcursor-dev libxcomposite-dev libxinerama-dev libxrandr-dev \
                       libxtst-dev libxss-dev libxdamage-dev libgl1-mesa-dev xvfb && \
    apt-get clean

# Build and install EFL for host build.
RUN git clone --depth 1 https://git.enlightenment.org/core/efl.git -b efl-1.25 /tmp/efl && \
    meson -Dbuild-examples=false -Dbuild-tests=false /tmp/efl /tmp/efl/build && \
    ninja -C /tmp/efl/build && \
    ninja -C /tmp/efl/build install && \
    rm -fr /tmp/efl
RUN ldconfig

# Start dbus service when running this container.
ENTRYPOINT /etc/init.d/dbus start && /bin/bash


############ for running plugins checking ##################################
RUN apt-get install -y python3-pip unzip && \
    apt-get clean
RUN pip3 install pyyaml
RUN git clone https://github.com/flutter-tizen/flutter-tizen.git /tmp/flutter-tizen

# Preprequisites
RUN apt-get update
RUN apt-get install -y acl bridge-utils gettext openvpn libfontconfig1 libglib2.0-0 libjpeg-turbo8 \
          libpixman-1-0 libpng16-16 libsdl1.2debian libsm6 libv4l-0 libx11-xcb1 libxcb-icccm4 \
          libxcb-image0 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xfixes0 libxi6 rpm2cpio cpio && \
    apt-get clean

# Tizen Studio
RUN apt-get update
RUN apt-get install -y pciutils zip libncurses5 python libpython2.7 && \
    apt-get clean

# create user for tizenstudio installation
RUN useradd -ms /bin/bash tizenstudiouser
USER tizenstudiouser
WORKDIR /home/tizenstudiouser

RUN curl http://download.tizen.org/sdk/Installer/tizen-studio_4.1/web-cli_Tizen_Studio_4.1_ubuntu-64.bin -o install.bin && \
        chmod a+x install.bin && \
        ./install.bin --accept-license $HOME/tizen-studio && \
        rm install.bin

# Tizen Studio packages
RUN $HOME/tizen-studio/package-manager/package-manager-cli.bin install \
          NativeCLI \
          NativeToolchain-Gcc-9.2 \
          WEARABLE-4.0-NativeAppDevelopment \
          WEARABLE-5.5-NativeAppDevelopment

# Create Tizen Studio certificate
RUN export PATH=$PATH:$HOME/tizen-studio/tools/ide/bin && \
        tizen certificate -a platform -p platform -f platform && \
        tizen security-profiles add -n platform -a $HOME/tizen-studio-data/keystore/author/platform.p12 -p platform

# Install flutter-tizen
RUN git clone --depth=1 https://github.com/flutter-tizen/flutter-tizen.git
ENV TIZEN_STUDIO_USER_HOME=/home/tizenstudiouser

ENV PATH=$PATH:$TIZEN_STUDIO_USER_HOME/flutter-tizen/bin

USER root
ENV PATH=$PATH:$TIZEN_STUDIO_USER_HOME/flutter-tizen/bin
ENV PATH=$PATH:$TIZEN_STUDIO_USER_HOME/tizen-studio/tools/ide/bin/
ENV TIZEN_SDK=$TIZEN_STUDIO_USER_HOME/tizen-studio

# Install DotNet
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        curl \
        ca-certificates \
        \
        # .NET dependencies
        libc6 \
        libgcc1 \
        libgssapi-krb5-2 \
        libicu66 \
        libssl1.1 \
        libstdc++6 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/*

RUN curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin -Channel Current -InstallDir /usr/share/dotnet \
    && ln -s /usr/share/dotnet/dotnet /usr/bin/dotnet

# build flutter-tizen tool as root (to gather all dependencies and make it ready to use)
RUN /home/tizenstudiouser/flutter-tizen/bin/flutter-tizen

############################################################################