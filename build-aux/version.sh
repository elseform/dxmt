#!/bin/sh
# Embed the source branch in development builds; keep release tag descriptions.
set -eu
source_root=$1
branch=$(git -C "$source_root" symbolic-ref --quiet --short HEAD || true)
if [ -n "$branch" ] && [ "$branch" != release ]; then
  printf '%s\n' "$branch"
else
  # Detached tagged builds and archives retain the previous version fallback.
  git -C "$source_root" describe --always
fi
