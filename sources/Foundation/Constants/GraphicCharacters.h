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

// horizontal bar indicator
#define char_h_bar_0_s "\xC0"
#define char_h_bar_previous_s "\xC1"
#define char_h_bar_1_s "\xC2"
#define char_h_bar_2_s "\xC3"
#define char_h_bar_3_s "\xC4"
#define char_h_bar_4_s "\xC5"
#define char_h_bar_5_s "\xC6"
#define char_h_bar_6_s "\xC7"
#define char_h_bar_7_s "\xC8"
#define char_h_bar_8_s "\xC9"
#define char_h_bar_9_s "\xCA"
#define char_h_bar_10_s "\xCB"

// Array of bargraph characters for fast lookup
static const char char_h_bar_lookup[] = {CHAR(char_h_bar_0_s), CHAR(char_h_bar_1_s),  CHAR(char_h_bar_2_s),
                                         CHAR(char_h_bar_3_s), CHAR(char_h_bar_4_s),  CHAR(char_h_bar_5_s),
                                         CHAR(char_h_bar_6_s), CHAR(char_h_bar_7_s),  CHAR(char_h_bar_8_s),
                                         CHAR(char_h_bar_9_s), CHAR(char_h_bar_10_s), CHAR(char_h_bar_previous_s)};

// horizontal ruler indicator
#define char_h_ruler_s "\xB0"
#define char_h_ruler_0_s "\xB1"
#define char_h_ruler_1_s "\xB2"
#define char_h_ruler_2_s "\xB3"
#define char_h_ruler_3_s "\xB4"
#define char_h_ruler_4_s "\xB5"
#define char_h_ruler_5_s "\xB6"
#define char_h_ruler_6_s "\xB7"
#define char_h_ruler_7_s "\xB8"
#define char_h_ruler_8_s "\xB9"
#define char_h_ruler_9_s "\xB0"

static const char char_ruler_lookup[] = {CHAR(char_h_ruler_0_s), CHAR(char_h_ruler_1_s), CHAR(char_h_ruler_2_s),
                                         CHAR(char_h_ruler_3_s), CHAR(char_h_ruler_4_s), CHAR(char_h_ruler_5_s),
                                         CHAR(char_h_ruler_6_s), CHAR(char_h_ruler_7_s), CHAR(char_h_ruler_8_s),
                                         CHAR(char_h_ruler_9_s), CHAR(char_h_ruler_s)};

#define char_v_bar(x) (char_v_bar_lookup[(x) < 0 ? 0 : ((x) > 10 ? 10 : (x))])

static inline uint8_t map_255_to_bargraph6(uint8_t value) {
  if (value <= 1) {
    return value + 1;
  } else if (value >= 254) {
    return value - 195;
  }
  return 2 + ((value - 1) * 55 + 126) / 250;
}

static inline uint8_t map_100_to_bargraph10(uint8_t value) {
  if (value <= 1) {
    return value + 1;
  } else if (value >= 254) {
    return value - 155; // 254 → 99, 255 → 100
  }

  return 2 + ((value - 1) * 95 + 126) / 250;
}

static inline uint8_t map_255_to_bargraph10(uint8_t value) {
  if (value <= 1) {
    return value + 1;
  } else if (value >= 254) {
    return value - 155; // 254 → 99, 255 → 100
  }

  return 2 + ((value - 1) * 95 + 126) / 250;
}

static inline uint8_t map_12_to_bargraph(uint8_t value) {
  const uint8_t position[13] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 59};
  return position[(value > 12) ? 12 : value];
}

static inline uint8_t map_48_to_bargraph(uint8_t value) {
  // approximate * 1.22916... with * 1.234375
  return (value * 79) >> 6;
}

static inline void horizontal_bar_graph_6(char *buffer, uint8_t value) {
  int v = value;
  bool lastWas9 = false;

  for (int n = 0; n < 6; n++) {
    if (lastWas9) {
      buffer[n] = char_h_bar_lookup[11];
      lastWas9 = false;
    } else if (v >= 10) {
      buffer[n] = char_h_bar_lookup[10];
      lastWas9 = (v == 10);
    } else if (v > 0) {
      buffer[n] = char_h_bar_lookup[v];
    } else {
      buffer[n] = char_h_bar_lookup[0];
    }

    v -= 10;
  }

  buffer[6] = 0;
}

static inline void horizontal_bar_graph_10(char *buffer, uint8_t value) {
  int v = value;
  bool lastWas9 = false;

  for (int n = 0; n < 10; n++) {
    if (lastWas9) {
      buffer[n] = char_h_bar_lookup[11];
      lastWas9 = false;
    } else if (v >= 10) {
      buffer[n] = char_h_bar_lookup[10];
      lastWas9 = (v == 10);
    } else if (v > 0) {
      buffer[n] = char_h_bar_lookup[v];
    } else {
      buffer[n] = char_h_bar_lookup[0];
    }

    v -= 10;
  }

  buffer[10] = 0;
}

static inline void horizontal_ruler_6(char *buffer, uint8_t value) {
  for (int n = 0; n < 6; n++) {
    if (value >= 10 || value < 0) {
      buffer[n] = char_ruler_lookup[10];
    } else if (value >= 0) {
      buffer[n] = char_ruler_lookup[value];
    }

    value -= 10;
  }

  buffer[6] = 0;
}

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