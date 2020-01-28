#!/usr/bin/env bash

UPSTREAM_FILE="$1"
DOWNLOAD_FILE="$(mktemp)"

wget -O "$DOWNLOAD_FILE" "$UPSTREAM_FILE"

tar -xf "$DOWNLOAD_FILE" --strip-components=1

rm "$DOWNLOAD_FILE"
