#!/bin/bash
PROJ="$(cd "$(dirname "$0")/.." && pwd)"

qemu-system-arm \
  -M vexpress-a9 \
  -kernel "$PROJ/qemu/zImage" \
  -dtb "$PROJ/qemu/vexpress-v2p-ca9.dtb" \
  -initrd "$PROJ/qemu/initramfs.cpio.gz" \
  -append "console=ttyAMA0 rdinit=/sbin/init" \
  -serial stdio \
  -display none \
  -m 256
