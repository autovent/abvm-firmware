#ifndef OLED_H
#define OLED_H

#include "platform.h"

class OLED {
public:
    OLED(I2C_HandleTypeDef *hi2c);

    void init();

    enum Screen {
        OVERVIEW,
        MENU_SET_ALARM,
        ALERT,
    };

    void set_screen(Screen screen);

    void set_overview_contents(int plateau_pres, int peak_pres, int tidal_vol, int resp_rate);

    void set_alarm_value(int peak_alarm);

    void set_alert_contents(const char *line1, const char *line2);

    void update();

private:
    Screen current_screen;

    I2C_HandleTypeDef *hi2c;

    // overview contents
    int plateau_pres, peak_pres, tidal_vol, resp_rate;

    int peak_alarm;

    char alert_line1[11];  // 10 chars + null
    char alert_line2[11];

    static void draw_h_line(int x1, int x2, int y, int color, int thickness);
    static void draw_v_line(int y1, int y2, int x, int color, int thickness);
};

#endif  // OLED_H
