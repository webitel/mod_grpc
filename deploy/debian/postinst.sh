#!/bin/sh
set -e

if command -v fs_cli >/dev/null 2>&1; then
    fs_cli -x "reload mod_grpc" || fs_cli -x "load mod_grpc" || true
fi
