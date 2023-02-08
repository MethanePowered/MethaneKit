FROM gitpod/workspace-full-vnc

# Install Linux dependencies
USER root
RUN apt-get update \
    && apt-get -y install build-essential ninja-build xcb libx11-dev libx11-xcb-dev libxcb-sync-dev libxcb-randr0-dev \
    && wget https://github.com/Kitware/CMake/releases/download/v3.25.2/cmake-3.25.2-linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod u+x /tmp/cmake-install.sh \
    && mkdir /opt/cmake-3.25.2 \
    && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.25.2 \
    && rm /tmp/cmake-install.sh \
    && ln -s /opt/cmake-3.25.2/bin/* /usr/local/bin \
    && apt-get clean && rm -rf /var/cache/apt/* && rm -rf /var/lib/apt/lists/* && rm -rf /tmp/*

USER gitpod
