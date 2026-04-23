#!/bin/bash
REPO="$(dirname "$0")"
make -C "$REPO/git-aware-nano" --quiet
"$REPO/git-aware-nano/build/nanoclone" "$@"
