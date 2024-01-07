#!/bin/bash
# CI helper script to install Linux packages, MethaneKit build prerequisites.
sudo apt update
sudo apt install xcb libx11-dev libx11-xcb-dev libxcb-sync-dev libxcb-randr0-dev p7zip $@
