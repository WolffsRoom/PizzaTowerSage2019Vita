#!/bin/sh
# Usage: addr2line.sh <elf> <addr...>
elf="$1"; shift
arm-vita-eabi-readelf -l "$elf" | grep -E "LOAD|Entry"
for a in "$@"; do
  echo "== $a"
  arm-vita-eabi-addr2line -f -C -i -e "$elf" "$a"
done

