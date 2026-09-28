#pragma once

// Keep glyphs as string literals so they can be concatenated at compile time.
// Use CHAR(...) when a single-byte character value is needed instead.

#define CHAR(s) ((s)[0])

#define char_battery_left_s "\x80"

#define gchar_key_low_off "\x06"
#define gchar_key_low_on "\x07"
#define gchar_key_end "\x08"
#define gchar_key_hi "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_1 "\x0c\x0d\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_3 "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_6 "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_8 "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_a "\x09\x0a\x0b\x09\x0a\x0a\x0b"

#define gchar_cup_0 "\x50"
#define gchar_cup_1 "\x51"
#define gchar_cup_2 "\x52"
#define gchar_cup_3 "\x53"

#define gchar_icon_yes "\x60"
#define gchar_icon_no "\x61"
#define gchar_icon_info "\x62"
#define gchar_icon_warning "\x63"

#define char_v_bar_0_s " "
#define char_v_bar_1_s "\xA0"
#define char_v_bar_2_s "\xA1"
#define char_v_bar_3_s "\xA2"
#define char_v_bar_4_s "\xA3"
#define char_v_bar_5_s "\xA4"
#define char_v_bar_6_s "\xA5"
#define char_v_bar_7_s "\xA6"
#define char_v_bar_8_s "\xA7"
#define char_v_bar_9_s "\xA8"
#define char_v_bar_10_s "\xA9"

static const char char_v_bar_lookup[] = {CHAR(char_v_bar_0_s), CHAR(char_v_bar_1_s), CHAR(char_v_bar_2_s),
                                         CHAR(char_v_bar_3_s), CHAR(char_v_bar_4_s), CHAR(char_v_bar_5_s),
                                         CHAR(char_v_bar_6_s), CHAR(char_v_bar_7_s), CHAR(char_v_bar_8_s),
                                         CHAR(char_v_bar_9_s), CHAR(char_v_bar_10_s)};

/*
static void drawPiano(uint16_t keys, char upper[8], char lower[8]) {
    // White keys: C D E F G A B
    constexpr uint8_t whiteBits[] = {0, 2, 4, 5, 7, 9, 11};

    // Black key immediately to the left/right of each white key.
    // 0xff means there is no black key.
    constexpr uint8_t leftBlack[]  = {0xff, 1, 3, 0xff, 6, 8, 10};
    constexpr uint8_t rightBlack[] = {1, 3, 0xff, 6, 8, 10, 0xff};

    for (uint8_t i = 0; i < 7; ++i) {
        const bool selected = keys & (1u << whiteBits[i]);
        lower[i] = selected ? 0x06 : 0x05;

        const bool leftSelected =
            leftBlack[i] != 0xff && (keys & (1u << leftBlack[i]));

        const bool rightSelected =
            rightBlack[i] != 0xff && (keys & (1u << rightBlack[i]));

        if (leftBlack[i] != 0xff && rightBlack[i] != 0xff) {
            if (leftSelected && rightSelected)
                upper[i] = 0x0f;
            else if (leftSelected)
                upper[i] = 0x0c;
            else if (rightSelected)
                upper[i] = 0x0d;
            else
                upper[i] = 0x09;
        } else if (rightBlack[i] != 0xff) {
            upper[i] = rightSelected ? 0x0b : 0x08;
        } else if (leftBlack[i] != 0xff) {
            upper[i] = leftSelected ? 0x0e : 0x0a;
        } else {
            upper[i] = 0x08;
        }
    }

    upper[7] = '\0';
    lower[7] = '\0';
}
*/