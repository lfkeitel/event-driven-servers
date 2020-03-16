#!/usr/bin/env bash

VERSION="$1"
UPSTREAM_FILE="http://www.pro-bono-publico.de/projects/src/DEVEL.${VERSION}.tar.bz2"
DOWNLOAD_FILE="$(mktemp)"

wget -O "$DOWNLOAD_FILE" "$UPSTREAM_FILE"

tar -xf "$DOWNLOAD_FILE" --strip-components=1

rm "$DOWNLOAD_FILE"
