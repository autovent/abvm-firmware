#include "oled.h"
#include <string.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"

OLED::OLED(I2C_HandleTypeDef *hi2c) : hi2c(hi2c) {}

void OLED::init() {
    // memcpy(&ssd1306_i2c_handle, hi2c, sizeof(I2C_HandleTypeDef));
    ssd1306_Init();
}

void OLED::set_screen(Screen screen) {
    current_screen = screen;
}

void OLED::set_overview_contents(int plateau_pres, int peak_pres, int tidal_vol, int resp_rate) {
    this->plateau_pres = plateau_pres;
    this->peak_pres = peak_pres;
    this->tidal_vol = tidal_vol;
    this->resp_rate = resp_rate;
}

void OLED::set_alarm_value(int alarm) {
    peak_alarm = alarm;
}

void OLED::set_alert_contents(const char *line1, const char *line2) {
    strcpy(alert_line1, line1);
    strcpy(alert_line2, line2);
    
    // ensure each line is not longer than 10 chars
    alert_line1[10] = '\0';
    alert_line2[10] = '\0';
}

void OLED::update() {
    ssd1306_Fill(Black);

    switch(current_screen) {
        case ALERT: {
            draw_h_line(4, SSD1306_WIDTH - 5, 4, White, 2);
            draw_h_line(4, SSD1306_WIDTH - 5, SSD1306_HEIGHT - 5, White, 2);
            draw_v_line(4, SSD1306_HEIGHT - 5, 4, White, 2);
            draw_v_line(4, SSD1306_HEIGHT - 5, SSD1306_WIDTH - 5, White, 2);

            ssd1306_SetCursor(31, 8);
            ssd1306_WriteString("ALERT!", Font_11x18, White);

            int x = 64 - (strlen(alert_line1) * 5.5f);
            ssd1306_SetCursor(x, 24);
            ssd1306_WriteString(alert_line1, Font_11x18, White);
            
            x = 64 - (strlen(alert_line2) * 5.5f);
            ssd1306_SetCursor(x, 40);
            ssd1306_WriteString(alert_line2, Font_11x18, White);

            break;
        }
        case MENU_SET_ALARM: {
            draw_h_line(4, SSD1306_WIDTH - 5, 4, White, 2);
            draw_h_line(4, SSD1306_WIDTH - 5, SSD1306_HEIGHT - 5, White, 2);
            draw_v_line(4, SSD1306_HEIGHT - 5, 4, White, 2);
            draw_v_line(4, SSD1306_HEIGHT - 4, SSD1306_WIDTH - 5, White, 2);

            ssd1306_SetCursor(9, 8);
            ssd1306_WriteString("Peak Alarm", Font_11x18, White);

            draw_h_line(9, 119, 26, White, 1);

            char alarm_val_str[3];  // 2 digits + null
            snprintf(alarm_val_str, sizeof(alarm_val_str), "%2d", peak_alarm);
            ssd1306_SetCursor(48, 32);
            ssd1306_WriteString(alarm_val_str, Font_16x26, White);

            ssd1306_SetCursor(84, 44);
            ssd1306_WriteString("cmH2O", Font_7x10, White);

            break;
        }
        case OVERVIEW:
        default: {
            char plateau_text[3];
            char peak_text[3];
            char vol_text[4];
            char rate_text[3];

            snprintf(plateau_text, sizeof(plateau_text), "%2d", plateau_pres);
            snprintf(peak_text, sizeof(peak_text), "%2d", peak_pres);
            snprintf(vol_text, sizeof(vol_text), "%3d", tidal_vol);
            snprintf(rate_text, sizeof(rate_text), "%2d", resp_rate);

            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(plateau_text, Font_16x26, White);

            ssd1306_SetCursor(0, 38);
            ssd1306_WriteString(peak_text, Font_16x26, White);

            ssd1306_SetCursor(80, 0);
            ssd1306_WriteString(vol_text, Font_16x26, White);

            ssd1306_SetCursor(96, 38);
            ssd1306_WriteString(rate_text, Font_16x26, White);

            draw_h_line(0, 128, 32, White, 2);
            draw_v_line(0, 64, 64, White, 2);

            break;
        }
    }

    ssd1306_UpdateScreen();
}

void OLED::draw_h_line(int x1, int x2, int y, int color, int thickness) {
    for (int t = 0; t < thickness; t++) {
        for (int x = x1; x <= x2; x++) {
            ssd1306_DrawPixel(x, y+t, (SSD1306_COLOR)color);
        }
    }
}

void OLED::draw_v_line(int y1, int y2, int x, int color, int thickness) {
    for (int t = 0; t < thickness; t++) {
        for (int y = y1; y <= y2; y++) {
            ssd1306_DrawPixel(x+t, y, (SSD1306_COLOR)color);
        }
    }
}
