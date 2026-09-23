#pragma once

// Keep glyphs as string literals so they can be concatenated at compile time.
// Use CHAR(...) when a single-byte character value is needed instead.

#define CHAR(s) ((s)[0])

#define char_battery_left_s "\x80"

#define gchar_key_low_off   "\x06"
#define gchar_key_low_on    "\x07"
#define gchar_key_end       "\x08"
#define gchar_key_hi        "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_1      "\x0c\x0d\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_3      "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_6      "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_8      "\x09\x0a\x0b\x09\x0a\x0a\x0b"
#define gchar_key_hi_a      "\x09\x0a\x0b\x09\x0a\x0a\x0b"

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