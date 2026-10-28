// Giao dien hien dai cho man hinh nho: thanh trang thai, man hinh chinh dang
// luoi icon (kieu dien thoai), man hinh chat dang bong bong. Chi dung LVGL 9,
// khong can file anh. Icon lay tu bo symbol co san cua LVGL (font Montserrat).
#pragma once

#include <lvgl.h>
#include <functional>

namespace aio {

struct UiFonts {
    const lv_font_t* text;   // chu thuong (phai co day du dau tieng Viet)
    const lv_font_t* small;  // chu nho cho caption
    const lv_font_t* icon;   // font co LV_SYMBOL_* (montserrat 14)
    const lv_font_t* icon_big;  // montserrat 24
};

class ModernUi {
public:
    using TileCallback = std::function<void(int tile_id)>;
    enum Tile { kTileChat = 0, kTileWifi, kTileTimer, kTileVolume, kTileBrightness, kTileInfo, kTileCount };

    // Goi khi da giu khoa LVGL (DisplayLockGuard).
    void Build(lv_obj_t* screen, int width, int height, const UiFonts& fonts);

    void SetTileCallback(TileCallback cb) { on_tile_ = std::move(cb); }
    void ShowHome();
    void ShowChat();
    void AddMessage(bool from_user, const char* text);
    void SetStatus(const char* text);
    void SetEmotion(const char* emotion);
    void SetWifi(bool connected);
    void SetBattery(int percent, bool charging);
    void Toast(const char* text);

private:
    struct TileCtx { ModernUi* ui; int id; };

    void BuildStatusBar();
    void BuildHome();
    void BuildChat();
    void UpdateClock();
    void TouchActivity();

    static void OnClockTick(lv_timer_t* t);
    static void OnIdle(lv_timer_t* t);
    static void OnToastDone(lv_timer_t* t);
    static void OnTileClick(lv_event_t* e);
    static void OnBackClick(lv_event_t* e);

    UiFonts fonts_{};
    int w_ = 0, h_ = 0, bar_h_ = 30;
    TileCallback on_tile_;
    TileCtx tile_ctx_[kTileCount]{};

    lv_obj_t* root_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* clock_lbl_ = nullptr;
    lv_obj_t* status_lbl_ = nullptr;
    lv_obj_t* wifi_lbl_ = nullptr;
    lv_obj_t* battery_lbl_ = nullptr;
    lv_obj_t* home_ = nullptr;
    lv_obj_t* greeting_lbl_ = nullptr;
    lv_obj_t* date_lbl_ = nullptr;
    lv_obj_t* chat_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* pill_dot_ = nullptr;
    lv_obj_t* pill_lbl_ = nullptr;
    lv_obj_t* toast_ = nullptr;
    lv_timer_t* idle_timer_ = nullptr;
};

}  // namespace aio
