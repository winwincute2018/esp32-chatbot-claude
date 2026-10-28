# aio28-modern-firmware

Firmware cho **AIDEEPEN ESP32-S3 2.8" ES3C28P** — tự host hoàn toàn.

[![Build Firmware](https://github.com/YOUR_USERNAME/aio28-modern-firmware/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/aio28-modern-firmware/actions/workflows/build.yml)

## Nạp firmware nhanh (sau khi build xong)

```bash
esptool.py --chip esp32s3 --port COM3 write_flash 0x0 merged-binary.bin
```
