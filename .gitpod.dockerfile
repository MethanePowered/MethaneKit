FROM gitpod/workspace-full-vnc

# Install Linux dependencies
USER root
RUN apt-get update \
    && apt-get -y install build-essential cmake ninja-build xcb libx11-dev libx11-xcb-dev libxcb-sync-dev libxcb-randr0-dev \
    && apt-get clean && rm -rf /var/cache/apt/* && rm -rf /var/lib/apt/lists/* && rm -rf /tmp/*

USER gitpod
