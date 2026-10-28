#include "modern_ui.h"

#include <cstdio>
#include <cstring>
#include <ctime>

namespace aio {

namespace {

constexpr uint32_t kBg = 0x0B0F1A;
constexpr uint32_t kBar = 0x0F1424;
constexpr uint32_t kSurface = 0x151B2B;
constexpr uint32_t kText = 0xF2F5FA;
constexpr uint32_t kSub = 0x8A93A8;
constexpr uint32_t kUserBubble = 0x3B82F6;
constexpr uint32_t kBotBubble = 0x1F2740;
constexpr int kMaxMessages = 16;
constexpr uint32_t kIdleBackMs = 20000;

lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }

struct TileDef {
    const char* symbol;
    const char* caption;
    uint32_t c1, c2;  // gradient doc
};

const TileDef kTileDefs[] = {
    {LV_SYMBOL_AUDIO,       "Trò chuyện", 0x6366F1, 0x8B5CF6},
    {LV_SYMBOL_WIFI,        "Wi-Fi",      0x06B6D4, 0x3B82F6},
    {LV_SYMBOL_BELL,        "Hẹn giờ",    0xF59E0B, 0xEF4444},
    {LV_SYMBOL_VOLUME_MAX,  "Âm lượng",   0x10B981, 0x059669},
    {LV_SYMBOL_EYE_OPEN,    "Độ sáng",    0xFACC15, 0xF97316},
    {LV_SYMBOL_SETTINGS,    "Thông tin",  0x64748B, 0x334155},
};

}  // namespace

void ModernUi::Build(lv_obj_t* screen, int width, int height, const UiFonts& fonts) {
    fonts_ = fonts;
    w_ = width;
    h_ = height;

    root_ = lv_obj_create(screen);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, w_, h_);
    lv_obj_set_pos(root_, 0, 0);
    lv_obj_set_style_bg_color(root_, C(kBg), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    content_ = lv_obj_create(root_);
    lv_obj_remove_style_all(content_);
    lv_obj_set_size(content_, w_, h_ - bar_h_);
    lv_obj_set_pos(content_, 0, bar_h_);
    lv_obj_remove_flag(content_, LV_OBJ_FLAG_SCROLLABLE);

    BuildStatusBar();
    BuildHome();
    BuildChat();

    toast_ = lv_label_create(root_);
    lv_obj_set_style_text_font(toast_, fonts_.small, 0);
    lv_obj_set_style_text_color(toast_, C(kText), 0);
    lv_obj_set_style_bg_color(toast_, C(0x2A3350), 0);
    lv_obj_set_style_bg_opa(toast_, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(toast_, 12, 0);
    lv_obj_set_style_pad_all(toast_, 6, 0);
    lv_obj_add_flag(toast_, LV_OBJ_FLAG_HIDDEN);

    idle_timer_ = lv_timer_create(&ModernUi::OnIdle, kIdleBackMs, this);
    lv_timer_pause(idle_timer_);
    lv_timer_create(&ModernUi::OnClockTick, 1000, this);

    UpdateClock();
    ShowHome();
}

void ModernUi::BuildStatusBar() {
    lv_obj_t* bar = lv_obj_create(root_);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, w_, bar_h_);
    lv_obj_set_style_bg_color(bar, C(kBar), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    clock_lbl_ = lv_label_create(bar);
    lv_obj_set_style_text_font(clock_lbl_, fonts_.text, 0);
    lv_obj_set_style_text_color(clock_lbl_, C(kText), 0);
    lv_obj_align(clock_lbl_, LV_ALIGN_LEFT_MID, 10, 0);

    status_lbl_ = lv_label_create(bar);
    lv_obj_set_width(status_lbl_, w_ * 40 / 100);
    lv_label_set_long_mode(status_lbl_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(status_lbl_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status_lbl_, fonts_.small, 0);
    lv_obj_set_style_text_color(status_lbl_, C(kSub), 0);
    lv_obj_align(status_lbl_, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(status_lbl_, "");

    battery_lbl_ = lv_label_create(bar);
    lv_obj_set_style_text_font(battery_lbl_, fonts_.icon, 0);
    lv_obj_set_style_text_color(battery_lbl_, C(kText), 0);
    lv_obj_align(battery_lbl_, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_label_set_text(battery_lbl_, "");

    wifi_lbl_ = lv_label_create(bar);
    lv_obj_set_style_text_font(wifi_lbl_, fonts_.icon, 0);
    lv_obj_set_style_text_color(wifi_lbl_, C(kSub), 0);
    lv_obj_align_to(wifi_lbl_, battery_lbl_, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_label_set_text(wifi_lbl_, LV_SYMBOL_WIFI);
}

void ModernUi::BuildHome() {
    home_ = lv_obj_create(content_);
    lv_obj_remove_style_all(home_);
    lv_obj_set_size(home_, w_, h_ - bar_h_);
    lv_obj_remove_flag(home_, LV_OBJ_FLAG_SCROLLABLE);

    greeting_lbl_ = lv_label_create(home_);
    lv_obj_set_style_text_font(greeting_lbl_, fonts_.text, 0);
    lv_obj_set_style_text_color(greeting_lbl_, C(kText), 0);
    lv_obj_align(greeting_lbl_, LV_ALIGN_TOP_LEFT, 14, 8);

    date_lbl_ = lv_label_create(home_);
    lv_obj_set_style_text_font(date_lbl_, fonts_.small, 0);
    lv_obj_set_style_text_color(date_lbl_, C(kSub), 0);
    lv_obj_align_to(date_lbl_, greeting_lbl_, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    // Luoi icon: 4 cot, tu xuong dong.
    const int cols = 4;
    const int tile_w = w_ / cols;
    const int tile_h = 78;
    lv_obj_t* grid = lv_obj_create(home_);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, w_, h_ - bar_h_ - 56);
    lv_obj_set_pos(grid, 0, 56);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (int i = 0; i < kTileCount; ++i) {
        const TileDef& def = kTileDefs[i];
        tile_ctx_[i] = {this, i};

        lv_obj_t* tile = lv_obj_create(grid);
        lv_obj_remove_style_all(tile);
        lv_obj_set_size(tile, tile_w, tile_h);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(tile, &ModernUi::OnTileClick, LV_EVENT_CLICKED, &tile_ctx_[i]);

        lv_obj_t* box = lv_obj_create(tile);
        lv_obj_remove_style_all(box);
        lv_obj_set_size(box, 50, 50);
        lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 2);
        lv_obj_set_style_radius(box, 14, 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(box, C(def.c1), 0);
        lv_obj_set_style_bg_grad_color(box, C(def.c2), 0);
        lv_obj_set_style_bg_grad_dir(box, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_opa(box, LV_OPA_70, LV_STATE_PRESSED);
        lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* icon = lv_label_create(box);
        lv_obj_set_style_text_font(icon, fonts_.icon_big, 0);
        lv_obj_set_style_text_color(icon, C(0xFFFFFF), 0);
        lv_label_set_text(icon, def.symbol);
        lv_obj_center(icon);

        lv_obj_t* cap = lv_label_create(tile);
        lv_obj_set_width(cap, tile_w - 4);
        lv_label_set_long_mode(cap, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(cap, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(cap, fonts_.small, 0);
        lv_obj_set_style_text_color(cap, C(kText), 0);
        lv_label_set_text(cap, def.caption);
        lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 56);
    }
}

void ModernUi::BuildChat() {
    const int content_h = h_ - bar_h_;
    const int foot_h = 40;

    chat_ = lv_obj_create(content_);
    lv_obj_remove_style_all(chat_);
    lv_obj_set_size(chat_, w_, content_h);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_SCROLLABLE);

    list_ = lv_obj_create(chat_);
    lv_obj_remove_style_all(list_);
    lv_obj_set_size(list_, w_, content_h - foot_h);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_, 8, 0);
    lv_obj_set_style_pad_row(list_, 6, 0);
    lv_obj_add_flag(list_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list_, LV_DIR_VER);

    lv_obj_t* foot = lv_obj_create(chat_);
    lv_obj_remove_style_all(foot);
    lv_obj_set_size(foot, w_, foot_h);
    lv_obj_set_pos(foot, 0, content_h - foot_h);
    lv_obj_set_style_bg_color(foot, C(kBar), 0);
    lv_obj_set_style_bg_opa(foot, LV_OPA_COVER, 0);
    lv_obj_remove_flag(foot, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* back = lv_obj_create(foot);
    lv_obj_remove_style_all(back);
    lv_obj_set_size(back, 32, 32);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(back, C(kSurface), 0);
    lv_obj_set_style_bg_opa(back, LV_OPA_COVER, 0);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(back, &ModernUi::OnBackClick, LV_EVENT_CLICKED, this);
    lv_obj_t* back_icon = lv_label_create(back);
    lv_obj_set_style_text_font(back_icon, fonts_.icon, 0);
    lv_obj_set_style_text_color(back_icon, C(kText), 0);
    lv_label_set_text(back_icon, LV_SYMBOL_HOME);
    lv_obj_center(back_icon);

    lv_obj_t* pill = lv_obj_create(foot);
    lv_obj_remove_style_all(pill);
    lv_obj_set_size(pill, w_ - 64, 28);
    lv_obj_align(pill, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pill, C(kSurface), 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

    pill_dot_ = lv_obj_create(pill);
    lv_obj_remove_style_all(pill_dot_);
    lv_obj_set_size(pill_dot_, 10, 10);
    lv_obj_align(pill_dot_, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_radius(pill_dot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pill_dot_, C(kSub), 0);
    lv_obj_set_style_bg_opa(pill_dot_, LV_OPA_COVER, 0);

    pill_lbl_ = lv_label_create(pill);
    lv_obj_set_width(pill_lbl_, w_ - 64 - 36);
    lv_label_set_long_mode(pill_lbl_, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(pill_lbl_, fonts_.small, 0);
    lv_obj_set_style_text_color(pill_lbl_, C(kText), 0);
    lv_obj_align(pill_lbl_, LV_ALIGN_LEFT_MID, 28, 0);
    lv_label_set_text(pill_lbl_, "");
}

void ModernUi::ShowHome() {
    lv_obj_remove_flag(home_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(chat_, LV_OBJ_FLAG_HIDDEN);
    if (idle_timer_) lv_timer_pause(idle_timer_);
    UpdateClock();
}

void ModernUi::ShowChat() {
    lv_obj_add_flag(home_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_HIDDEN);
    TouchActivity();
}

void ModernUi::TouchActivity() {
    if (!idle_timer_) return;
    lv_timer_reset(idle_timer_);
    lv_timer_resume(idle_timer_);
}

void ModernUi::AddMessage(bool from_user, const char* text) {
    if (!text || !*text) return;
    ShowChat();

    lv_obj_t* row = lv_obj_create(list_);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, from_user ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    const int max_bubble = w_ * 78 / 100;
    lv_obj_t* bubble = lv_obj_create(row);
    lv_obj_remove_style_all(bubble);
    lv_obj_set_width(bubble, LV_SIZE_CONTENT);
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(bubble, max_bubble, 0);
    lv_obj_set_style_pad_all(bubble, 8, 0);
    lv_obj_set_style_radius(bubble, 14, 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bubble, C(from_user ? kUserBubble : kBotBubble), 0);
    lv_obj_remove_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(bubble);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_max_width(label, max_bubble - 16, 0);
    lv_obj_set_style_text_font(label, fonts_.text, 0);
    lv_obj_set_style_text_color(label, C(kText), 0);
    lv_label_set_text(label, text);

    while (lv_obj_get_child_count(list_) > kMaxMessages) {
        lv_obj_delete(lv_obj_get_child(list_, 0));
    }
    lv_obj_scroll_to_view(row, LV_ANIM_ON);
}

void ModernUi::SetStatus(const char* text) {
    if (pill_lbl_) lv_label_set_text(pill_lbl_, text ? text : "");
    if (status_lbl_) lv_label_set_text(status_lbl_, text ? text : "");
}

void ModernUi::SetEmotion(const char* emotion) {
    // Khong co font emoji, nen dung cham mau trong "vien thuoc" de the hien cam xuc.
    uint32_t color = kSub;
    if (emotion) {
        if (!strcmp(emotion, "happy") || !strcmp(emotion, "laughing") || !strcmp(emotion, "funny")) color = 0x22C55E;
        else if (!strcmp(emotion, "thinking") || !strcmp(emotion, "confused")) color = 0xF59E0B;
        else if (!strcmp(emotion, "sad") || !strcmp(emotion, "crying")) color = 0x60A5FA;
        else if (!strcmp(emotion, "angry")) color = 0xEF4444;
    }
    if (pill_dot_) lv_obj_set_style_bg_color(pill_dot_, C(color), 0);
}

void ModernUi::SetWifi(bool connected) {
    if (wifi_lbl_) lv_obj_set_style_text_color(wifi_lbl_, C(connected ? kText : 0x4B5563), 0);
}

void ModernUi::SetBattery(int percent, bool charging) {
    if (!battery_lbl_) return;
    if (percent < 0) {
        lv_label_set_text(battery_lbl_, "");
        return;
    }
    const char* sym = percent > 85 ? LV_SYMBOL_BATTERY_FULL
                    : percent > 60 ? LV_SYMBOL_BATTERY_3
                    : percent > 35 ? LV_SYMBOL_BATTERY_2
                    : percent > 15 ? LV_SYMBOL_BATTERY_1
                                   : LV_SYMBOL_BATTERY_EMPTY;
    lv_label_set_text_fmt(battery_lbl_, "%s%s", charging ? LV_SYMBOL_CHARGE " " : "", sym);
    lv_obj_set_style_text_color(battery_lbl_, C(percent <= 15 && !charging ? 0xEF4444 : kText), 0);
    lv_obj_align(battery_lbl_, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_align_to(wifi_lbl_, battery_lbl_, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

void ModernUi::Toast(const char* text) {
    if (!toast_ || !text) return;
    lv_label_set_text(toast_, text);
    lv_obj_remove_flag(toast_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(toast_, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_move_foreground(toast_);
    lv_timer_t* t = lv_timer_create(&ModernUi::OnToastDone, 1500, this);
    lv_timer_set_repeat_count(t, 1);  // tu huy sau 1 lan
}

void ModernUi::UpdateClock() {
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    const bool synced = tm_now.tm_year >= 120;  // >= nam 2020

    if (clock_lbl_) {
        char buf[8];
        if (synced) strftime(buf, sizeof(buf), "%H:%M", &tm_now);
        else snprintf(buf, sizeof(buf), "--:--");
        lv_label_set_text(clock_lbl_, buf);
    }
    if (greeting_lbl_) {
        const int h = tm_now.tm_hour;
        const char* g = !synced ? "Xin chào"
                      : h < 11 ? "Chào buổi sáng"
                      : h < 14 ? "Chào buổi trưa"
                      : h < 18 ? "Chào buổi chiều"
                               : "Chào buổi tối";
        lv_label_set_text(greeting_lbl_, g);
    }
    if (date_lbl_) {
        char d[24];
        if (synced) snprintf(d, sizeof(d), "%02d/%02d/%04d", tm_now.tm_mday, tm_now.tm_mon + 1, tm_now.tm_year + 1900);
        else d[0] = '\0';
        lv_label_set_text(date_lbl_, d);
    }
}

void ModernUi::OnClockTick(lv_timer_t* t) {
    static_cast<ModernUi*>(lv_timer_get_user_data(t))->UpdateClock();
}

void ModernUi::OnIdle(lv_timer_t* t) {
    static_cast<ModernUi*>(lv_timer_get_user_data(t))->ShowHome();
}

void ModernUi::OnToastDone(lv_timer_t* t) {
    auto* self = static_cast<ModernUi*>(lv_timer_get_user_data(t));
    lv_obj_add_flag(self->toast_, LV_OBJ_FLAG_HIDDEN);
}

void ModernUi::OnTileClick(lv_event_t* e) {
    auto* ctx = static_cast<TileCtx*>(lv_event_get_user_data(e));
    if (ctx->ui->on_tile_) ctx->ui->on_tile_(ctx->id);
}

void ModernUi::OnBackClick(lv_event_t* e) {
    static_cast<ModernUi*>(lv_event_get_user_data(e))->ShowHome();
}

}  // namespace aio
