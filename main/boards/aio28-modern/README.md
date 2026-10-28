# Board: aio28-modern — AIDEEPEN ESP32-S3 2.8" ES3C28P

Firmware xiaozhi-esp32 tùy biến cho **AIDEEPEN ES3C28P**, hoàn toàn tự host,  
không phụ thuộc xiaozhi.vn, tenclass.net hay bất kỳ bên thứ ba nào.

## Phần cứng đã xác nhận
| Linh kiện | Chi tiết |
|---|---|
| MCU | ESP32-S3, Flash 16MB, PSRAM 8MB |
| Codec âm thanh | ES8311 @ I2C addr 0x18 |
| Màn hình | ILI9341V 240×320 (SPI) |
| Cảm ứng | FT6336G (I2C, addr 0x38) |
| Thẻ nhớ | SDMMC 4-bit tại /sdcard |
| Pin | ADC IO9 (chia đôi điện áp) |
| LED RGB | WS2812 IO42 |

## GPIO đầy đủ (nguồn: lcdwiki.com ES3C28P_Demo_Instructions)
| Chức năng | GPIO |
|---|---|
| LCD CS / DC / SCK / MOSI / MISO | 10 / 46 / 12 / 11 / 13 |
| LCD Backlight | 45 |
| I2C SDA / SCL (codec + touch) | 16 / 15 |
| I2S MCLK / BCLK / WS / DO / DI | 4 / 5 / 7 / 8 / 6 |
| PA enable (LOW=on) | 1 |
| Touch RST / INT | 18 / 17 |
| SD CLK / CMD / D0-D3 | 38 / 40 / 39 / 41 / 48 / 47 |
| BOOT button | 0 |
| Battery ADC | 9 |
| RGB LED | 42 |

## Tính năng mới so với firmware gốc
- ✅ Giao diện LVGL hiện đại: lưới icon, bong bóng chat, thanh trạng thái
- ✅ Cảm ứng FT6336G: chạm icon để kích hoạt tính năng
- ✅ Hiển thị cảm xúc AI bằng màu sắc
- ✅ Báo thức bằng giọng nói qua MCP
- ✅ Phát nhạc từ thẻ SD
- ✅ Đọc pin qua ADC
- ✅ Toast thông báo
- ❌ Bỏ: license check, MEDIA_BLOCKED, kết nối portal.xiaozhi.vn

## Cách build (5 bước)

### Bước 1 — Cài ESP-IDF v5.5
```bash
git clone --recursive https://github.com/espressif/esp-idf.git ~/esp-idf
cd ~/esp-idf && git checkout v5.5 && ./install.sh esp32s3
. ~/esp-idf/export.sh
```

### Bước 2 — Clone firmware gốc
```bash
git clone --recursive https://github.com/78/xiaozhi-esp32.git
cd xiaozhi-esp32
```

### Bước 3 — Copy board này vào
```bash
cp -r /duong/dan/aio28-modern main/boards/aio28-modern
```

Thêm vào `main/Kconfig.projbuild` trong block `choice BOARD_TYPE`:
```kconfig
config BOARD_TYPE_AIO28_MODERN
    bool "AIDEEPEN ES3C28P 2.8 Modern UI"
    depends on IDF_TARGET_ESP32S3
```

Thêm vào `main/CMakeLists.txt`:
```cmake
elseif(CONFIG_BOARD_TYPE_AIO28_MODERN)
    set(BOARD_DIR "aio28-modern")
    set(BUILTIN_TEXT_FONT font_puhui_basic_14_1)
    set(BUILTIN_ICON_FONT font_awesome_14_1)
    set(DEFAULT_EMOJI_COLLECTION twemoji_32)
```

### Bước 4 — Build
```bash
idf.py set-target esp32s3
idf.py menuconfig
# Chọn: Xiaozhi Assistant → Board Type → AIDEEPEN ES3C28P 2.8 Modern UI
# Chọn: Xiaozhi Assistant → Server → điền URL máy chủ của bạn
idf.py build
```

File đầu ra: `build/merged-binary.bin` (nạp tại địa chỉ 0x0)

### Bước 5 — Nạp firmware
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  write_flash 0x0 build/merged-binary.bin
```

Hoặc trên Windows:
```
esptool.exe --chip esp32s3 --port COM3 write_flash 0x0 build\merged-binary.bin
```

## Máy chủ tự host (xiaozhi-esp32-server)
```bash
git clone https://github.com/xinnan-tech/xiaozhi-esp32-server
cd xiaozhi-esp32-server
cp .env.example .env
# Sửa .env: chọn LLM, ASR, TTS
docker compose up -d
```
Khuyến nghị cho tiếng Việt:
- **ASR**: FunASR (local) hoặc Whisper
- **LLM**: Ollama + Qwen2.5 (local) hoặc OpenAI-compatible API
- **TTS**: Piper (local) hoặc Edge TTS
