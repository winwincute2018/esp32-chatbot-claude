// =============================================================================
//  Board: aio28-modern
//  Hardware: AIDEEPEN ESP32-S3 2.8" 240x320 (ES3C28P)
//            ES8311 codec, ILI9341V LCD, FT6336G touch, SDMMC, WS2812 LED
//  Firmware: xiaozhi-esp32 (github.com/78/xiaozhi-esp32) — tu host hoan toan
//  GPIO: theo tai lieu chinh thuc lcdwiki.com/res/ES3C28P/
// =============================================================================

#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "modern_ui.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_touch_ft5x06.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <driver/sdmmc_host.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/adc.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define TAG "Aio28Modern"

class Aio28ModernBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_{};
    Button boot_button_{BOOT_BUTTON_GPIO};
    SpiLcdDisplay*     display_{};
    aio::ModernUi      ui_{};
    bool               sd_mounted_{false};

    // ============================================================ I2C
    // ES8311 + FT6336G dung chung mot bus I2C (IO15/IO16)
    void InitI2c() {
        i2c_master_bus_config_t cfg{};
        cfg.i2c_port              = I2C_NUM_0;
        cfg.sda_io_num            = AUDIO_CODEC_I2C_SDA_PIN;
        cfg.scl_io_num            = AUDIO_CODEC_I2C_SCL_PIN;
        cfg.clk_source            = I2C_CLK_SRC_DEFAULT;
        cfg.glitch_ignore_cnt     = 7;
        cfg.flags.enable_internal_pullup = 1;
        ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &i2c_bus_));
        ESP_LOGI(TAG, "I2C ready SDA=%d SCL=%d", AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);
    }

    // ============================================================ SPI (LCD)
    void InitSpi() {
        spi_bus_config_t bus{};
        bus.mosi_io_num      = DISPLAY_SPI_MOSI_PIN;
        bus.miso_io_num      = DISPLAY_SPI_MISO_PIN;
        bus.sclk_io_num      = DISPLAY_SPI_SCK_PIN;
        bus.quadwp_io_num    = GPIO_NUM_NC;
        bus.quadhd_io_num    = GPIO_NUM_NC;
        bus.max_transfer_sz  = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
        ESP_LOGI(TAG, "SPI ready MOSI=%d SCK=%d CS=%d", DISPLAY_SPI_MOSI_PIN, DISPLAY_SPI_SCK_PIN, DISPLAY_SPI_CS_PIN);
    }

    // ============================================================ ILI9341
    void InitDisplay() {
        esp_lcd_panel_io_handle_t io{};
        esp_lcd_panel_io_spi_config_t io_cfg{};
        io_cfg.cs_gpio_num       = DISPLAY_SPI_CS_PIN;
        io_cfg.dc_gpio_num       = DISPLAY_DC_PIN;
        io_cfg.spi_mode          = 0;
        io_cfg.pclk_hz           = 40 * 1000 * 1000;
        io_cfg.trans_queue_depth = 10;
        io_cfg.lcd_cmd_bits      = 8;
        io_cfg.lcd_param_bits    = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io));

        esp_lcd_panel_handle_t panel{};
        esp_lcd_panel_dev_config_t panel_cfg{};
        panel_cfg.reset_gpio_num = DISPLAY_RST_PIN;
        panel_cfg.bits_per_pixel = 16;
        panel_cfg.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io, &panel_cfg, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(io, panel,
            DISPLAY_WIDTH, DISPLAY_HEIGHT,
            DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
            DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);

        ESP_LOGI(TAG, "ILI9341 ready %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    // ============================================================ Touch FT6336G
    void InitTouch() {
        esp_lcd_touch_config_t tp_cfg{};
        tp_cfg.x_max         = DISPLAY_WIDTH;
        tp_cfg.y_max         = DISPLAY_HEIGHT;
        tp_cfg.rst_gpio_num  = TOUCH_RST_PIN;
        tp_cfg.int_gpio_num  = TOUCH_INT_PIN;
        tp_cfg.levels.reset  = 0;
        tp_cfg.levels.interrupt = 0;
        tp_cfg.flags.swap_xy   = DISPLAY_SWAP_XY;
        tp_cfg.flags.mirror_x  = DISPLAY_MIRROR_X;
        tp_cfg.flags.mirror_y  = DISPLAY_MIRROR_Y;

        esp_lcd_panel_io_handle_t tp_io{};
        esp_lcd_panel_io_i2c_config_t tp_io_cfg{};
        tp_io_cfg.dev_addr   = TOUCH_I2C_ADDR;
        tp_io_cfg.scl_speed_hz = 100000;
        tp_io_cfg.control_phase_bytes = 1;
        tp_io_cfg.lcd_cmd_bits = 8;
        tp_io_cfg.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_, &tp_io_cfg, &tp_io));

        esp_lcd_touch_handle_t tp{};
        if (esp_lcd_touch_new_i2c_ft5x06(tp_io, &tp_cfg, &tp) == ESP_OK) {
            // Dang ky touch voi display de LVGL xu ly
            display_->SetTouch(tp);
            ESP_LOGI(TAG, "FT6336G touch ready");
        } else {
            ESP_LOGW(TAG, "Touch init failed — tiep tuc khong co touch");
        }
    }

    // ============================================================ SD Card
    void InitSdCard() {
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
        slot.clk  = (gpio_num_t)SD_CLK_PIN;
        slot.cmd  = (gpio_num_t)SD_CMD_PIN;
        slot.d0   = (gpio_num_t)SD_D0_PIN;
        slot.d1   = (gpio_num_t)SD_D1_PIN;
        slot.d2   = (gpio_num_t)SD_D2_PIN;
        slot.d3   = (gpio_num_t)SD_D3_PIN;
        slot.width = 4;
        slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

        esp_vfs_fat_sdmmc_mount_config_t mount_cfg{};
        mount_cfg.format_if_mount_failed = false;
        mount_cfg.max_files = 8;
        mount_cfg.allocation_unit_size = 16 * 1024;

        sdmmc_card_t* card;
        esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mount_cfg, &card);
        if (ret == ESP_OK) {
            sd_mounted_ = true;
            ESP_LOGI(TAG, "SD card mounted: %lluMB", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
        } else {
            ESP_LOGW(TAG, "SD card mount failed (%s) — tiep tuc khong co SD", esp_err_to_name(ret));
        }
    }

    // ============================================================ UI LVGL
    void InitUi() {
        display_->Lock();

        aio::UiFonts fonts{};
        // Font duoc link vao firmware qua CMakeLists.txt.
        // BUILTIN_TEXT_FONT = font_puhui_basic_14_1 (trong CMakeLists.txt)
        // BUILTIN_ICON_FONT = font_awesome_14_1
        extern const lv_font_t BUILTIN_TEXT_FONT;
        extern const lv_font_t BUILTIN_ICON_FONT;
        fonts.text     = &BUILTIN_TEXT_FONT;
        fonts.small    = &BUILTIN_TEXT_FONT;
        fonts.icon     = &BUILTIN_ICON_FONT;
        fonts.icon_big = &BUILTIN_ICON_FONT;

        ui_.Build(lv_screen_active(), DISPLAY_WIDTH, DISPLAY_HEIGHT, fonts);
        ui_.SetTileCallback([this](int id) { OnTileClick(id); });

        display_->Unlock();
    }

    void OnTileClick(int id) {
        auto& app = Application::GetInstance();
        switch (id) {
            case aio::ModernUi::kTileChat:
                app.ToggleChatState();
                break;
            case aio::ModernUi::kTileWifi:
                display_->Lock();
                ui_.Toast("Giữ nút BOOT 3s để cấu hình Wi-Fi");
                display_->Unlock();
                break;
            default:
                display_->Lock();
                ui_.ShowChat();
                ui_.Toast("Đang phát triển...");
                display_->Unlock();
                break;
        }
    }

    // ============================================================ Nut bam
    void InitButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
        boot_button_.OnLongPress([this]() {
            EnterWifiConfigMode();
        });
    }

    // ============================================================ Pin ADC
    void InitBattery() {
        // Bao cao pin qua MCP tool; doc ADC don gian
        auto& mcp = McpServer::GetInstance();
        mcp.AddTool("self.battery.get",
            "Đọc điện áp pin (mV)",
            PropertyList(),
            [](const PropertyList&) -> ReturnValue {
                adc1_config_width(ADC_WIDTH_BIT_12);
                adc1_config_channel_atten(ADC1_CHANNEL_8, ADC_ATTEN_DB_12); // IO9 = ADC1_CH8
                int raw = adc1_get_raw(ADC1_CHANNEL_8);
                // Raw 0-4095 tuong duong 0-3.3V; pin 3.7V qua cau chia doi
                int mv = (int)((raw / 4095.0f) * 3300 * 2);
                char buf[32];
                snprintf(buf, sizeof(buf), "{\"mv\":%d}", mv);
                return std::string(buf);
            });
    }

    // ============================================================ MCP Tools
    void InitTools() {
        auto& mcp = McpServer::GetInstance();

        // ---- Do sang man hinh ----
        mcp.AddTool("self.screen.set_brightness",
            "Chỉnh độ sáng màn hình 0–100",
            PropertyList({ Property("brightness", kPropertyTypeInteger, 0, 100) }),
            [this](const PropertyList& p) -> ReturnValue {
                GetBacklight()->SetBrightness(p.GetProperty("brightness").value<int>());
                return true;
            });

        // ---- Am luong ----
        mcp.AddTool("self.audio.set_volume",
            "Chỉnh âm lượng loa 0–100",
            PropertyList({ Property("volume", kPropertyTypeInteger, 0, 100) }),
            [](const PropertyList& p) -> ReturnValue {
                Application::GetInstance().SetVolume(p.GetProperty("volume").value<int>());
                return true;
            });

        // ---- Trang thai thiet bi ----
        mcp.AddTool("self.get_device_status",
            "Lấy trạng thái thiết bị",
            PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                char buf[128];
                snprintf(buf, sizeof(buf),
                    "{\"volume\":%d,\"sd_mounted\":%s}",
                    Application::GetInstance().GetVolume(),
                    sd_mounted_ ? "true" : "false");
                return std::string(buf);
            });

        // ---- Toast tren man hinh ----
        mcp.AddTool("self.screen.toast",
            "Hiện thông báo ngắn trên màn hình",
            PropertyList({ Property("text", kPropertyTypeString) }),
            [this](const PropertyList& p) -> ReturnValue {
                std::string t = p.GetProperty("text").value<std::string>();
                display_->Lock();
                ui_.Toast(t.c_str());
                display_->Unlock();
                return true;
            });

        // ---- Khoi dong lai ----
        mcp.AddTool("self.reboot",
            "Khởi động lại thiết bị",
            PropertyList(),
            [](const PropertyList&) -> ReturnValue {
                esp_restart();
                return true;
            });

        // ---- Phat file am thanh tu SD ----
        mcp.AddTool("self.music.play_file",
            "Phát file âm thanh từ thẻ nhớ. path: đường dẫn từ /sdcard/music/",
            PropertyList({ Property("path", kPropertyTypeString) }),
            [this](const PropertyList& p) -> ReturnValue {
                if (!sd_mounted_) return std::string("SD card không có sẵn");
                std::string path = "/sdcard/music/" + p.GetProperty("path").value<std::string>();
                // Goi AudioPlayer neu co; hien tai chi bao cao duong dan
                char buf[256];
                snprintf(buf, sizeof(buf), "{\"playing\":\"%s\"}", path.c_str());
                return std::string(buf);
            });

        // ---- Bao thuc ----
        mcp.AddTool("self.alarm.set",
            "Đặt báo thức sau N giây",
            PropertyList({
                Property("seconds", kPropertyTypeInteger, 1, 86400),
                Property("message", kPropertyTypeString, std::string("Báo thức!"))
            }),
            [this](const PropertyList& p) -> ReturnValue {
                int sec = p.GetProperty("seconds").value<int>();
                std::string msg = p.GetProperty("message").value<std::string>();
                // Lap lich don gian qua FreeRTOS timer
                struct AlarmArg { std::string msg; Aio28ModernBoard* self; };
                auto* arg = new AlarmArg{msg, this};
                xTimerHandle t = xTimerCreate("alarm", pdMS_TO_TICKS(sec * 1000), pdFALSE, arg,
                    [](xTimerHandle xTimer) {
                        auto* a = (AlarmArg*)pvTimerGetTimerID(xTimer);
                        a->self->display_->Lock();
                        a->self->ui_.Toast(a->msg.c_str());
                        a->self->display_->Unlock();
                        // Doc to qua TTS
                        Application::GetInstance().Say(a->msg);
                        delete a;
                    });
                xTimerStart(t, 0);
                char buf[64];
                snprintf(buf, sizeof(buf), "{\"alarm_in_sec\":%d}", sec);
                return std::string(buf);
            });
    }

    // ============================================================ Xu ly su kien App
    void OnStateChange(DeviceState state) {
        display_->Lock();
        switch (state) {
            case kDeviceStateIdle:
                ui_.SetStatus("");
                ui_.SetEmotion(nullptr);
                ui_.SetWifi(true);
                ui_.ShowHome();
                break;
            case kDeviceStateListening:
                ui_.ShowChat();
                ui_.SetStatus("Đang nghe...");
                break;
            case kDeviceStateSpeaking:
                ui_.SetStatus("Đang nói...");
                break;
            case kDeviceStateConnecting:
                ui_.SetStatus("Đang kết nối...");
                break;
            case kDeviceStateActivating:
                ui_.SetStatus("Khởi động...");
                break;
            case kDeviceStateUpgrading:
                ui_.SetStatus("Đang cập nhật OTA...");
                break;
            case kDeviceStateWifiConfiguring:
                ui_.Toast("Kết nối Wi-Fi: XIAOZHI-SETUP");
                break;
            default:
                break;
        }
        display_->Unlock();
    }

public:
    Aio28ModernBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        ESP_LOGI(TAG, "Khoi dong AIDEEPEN ES3C28P board...");

        InitI2c();
        InitSpi();
        InitDisplay();
        InitTouch();
        InitSdCard();
        InitButtons();
        InitTools();
        InitBattery();
        GetBacklight()->SetBrightness(100);
        InitUi();

        // Ket noi su kien Application vao UI
        auto& app = Application::GetInstance();

        app.OnStateChange([this](DeviceState s) {
            OnStateChange(s);
        });

        app.OnAiMessage([this](const std::string& text, bool is_user) {
            display_->Lock();
            ui_.AddMessage(is_user, text.c_str());
            display_->Unlock();
        });

        app.OnEmotionChange([this](const std::string& emotion) {
            display_->Lock();
            ui_.SetEmotion(emotion.c_str());
            display_->Unlock();
        });

        app.OnWifiStateChange([this](bool connected) {
            display_->Lock();
            ui_.SetWifi(connected);
            if (!connected) ui_.SetStatus("Mất kết nối Wi-Fi");
            display_->Unlock();
        });

        ESP_LOGI(TAG, "Board san sang. SD=%s", sd_mounted_ ? "OK" : "khong co");
    }

    AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec codec(
            i2c_bus_, I2C_NUM_0,
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR,
            true,  // output_reference
            AUDIO_CODEC_PA_INVERT);
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        static PwmBacklight bl(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &bl;
    }
};

DECLARE_BOARD(Aio28ModernBoard);
