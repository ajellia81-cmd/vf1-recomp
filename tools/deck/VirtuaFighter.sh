#!/bin/sh
# Launcher for Steam: runs the game from its own folder (saves go in data/).
cd "$(dirname "$0")" && exec ./vf "$@"
