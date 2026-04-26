#!/bin/bash
# Launch shijima-qt with proper library paths

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Linux: set library path for libunarr
if [ "$(uname)" = "Linux" ]; then
    export LD_LIBRARY_PATH="$SCRIPT_DIR/libshimejifinder/build/unarr${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

# macOS: set library path for libunarr
if [ "$(uname)" = "Darwin" ]; then
    export DYLD_LIBRARY_PATH="$SCRIPT_DIR/libshimejifinder/build/unarr${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
fi

exec "$SCRIPT_DIR/shijima-qt" "$@"
