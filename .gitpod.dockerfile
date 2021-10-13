FROM gitpod/workspace-full-vnc

# Install Linux dependencies of Methane Kit
RUN sudo apt-get update && sudo apt-get install xcb libx11-dev libx11-xcb-dev