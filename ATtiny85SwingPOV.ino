#include <Arduino.h>
#include <avr/power.h>
#include <Adafruit_NeoPixel.h>

#include "SwingPov.h"
#include "data.h"

/*  Defines  */

#define BUTTON_PIN      0
#define TILT_ENABLE_PIN 1
#define TILT_STATUS_PIN 2
#define PIXELS_PIN      3

#define NUMBER_PIXELS   8
#define HUE_MAX         1536

#define MODE_MAX        ((sizeof(configTable) / sizeof(CONFIG_T)))

/*  Local Functions  */

static void changeMode(uint8_t mode);
static uint8_t getFontPattern(char letter, uint8_t column);
static uint32_t hsv2Color(uint16_t hue, uint8_t sat, uint8_t val);
static void drawMargin(void);

static void initMessage(void);
static void changeDirectionMessage(int8_t direction);
static void drawColumnMessage(uint8_t position);

static void initCounter(void);
static void changeDirectionCounter(int8_t direction);
static void drawColumnCounter(uint8_t position);

static void initOBONO(void);
static void changeDirectionOBONO(int8_t direction);
static void drawColumnOBONO(uint8_t position);
static void drawMarginOBONO(void);

/*  Local Constants  */

PROGMEM static const CONFIG_T configTable[] = {
    { 30, 9,  initMessage, changeDirectionMessage, drawColumnMessage, drawMargin      },
    { 24, 12, initCounter, changeDirectionCounter, drawColumnCounter, drawMargin      },
    { 16, 16, initOBONO,   changeDirectionOBONO,   drawColumnOBONO,   drawMarginOBONO },
};

/*  Local Variables  */

static Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);

static uint32_t palette[12];
static uint16_t scroll, counter, length;
static uint8_t  mode, lastButtonState;
static char     numerals[5];

/*-----------------------------------------------------------------------------*/

void setup(void)
{

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    lastButtonState = digitalRead(BUTTON_PIN);

    pixels.begin();
    pixels.fill();
    initSwingPov(TILT_ENABLE_PIN, TILT_STATUS_PIN);
    changeMode(0);
    mode = 0;
}

void loop(void)
{
    uint8_t currentButtonState = digitalRead(BUTTON_PIN);
    if (lastButtonState != currentButtonState) {
        lastButtonState = currentButtonState;
        if (currentButtonState == LOW) {
            mode = (mode + 1) % MODE_MAX;
            changeMode(mode);
        }
    }
    updateSwingPov();
    int32_t delayTime = getNextTime() - millis();
    if (delayTime > 0) delay(delayTime);
}

/*---------------------------------------------------------------------------*/

static void changeMode(uint8_t mode)
{
    CONFIG_T config;
    memcpy_P(&config, &configTable[mode], sizeof(config));
    configSwingPov(config);
}

static uint8_t getFontPattern(char letter, uint8_t column)
{
    if (letter < '!' || letter > '~' || column >= FONT_WIDTH - 1) return 0;
    return pgm_read_byte(&font[letter - '!'][column]);
}

static uint32_t hsv2Color(uint16_t hue, uint8_t sat, uint8_t val)
{
    int16_t hue2 = hue % HUE_MAX;
    uint16_t r = 0, g = 0, b = 0;
    if (hue2 <= 512 || hue2 >= 1024) {
        r = abs(768 - hue2) - 256;
        if (r >= 256) r = 256;
    }
    if (hue2 <= 1024) {
        g = 512 - abs(512 - hue2);
        if (g >= 256) g = 256;
    }
    if (hue2 >= 512) {
        b = 512 - abs(1024 - hue2);
        if (b >= 256) b = 256;
    }
    uint16_t z = 256 - sat;
    r = (r * sat >> 8) + z;
    g = (g * sat >> 8) + z;
    b = (b * sat >> 8) + z;
    return pixels.Color(r * val >> 8, g * val >> 8, b * val >> 8);
}

static void drawMargin(void)
{
    pixels.fill();
    pixels.show();
}

/*---------------------------------------------------------------------------*/

static void initMessage(void)
{
    scroll = 0;
}

static void changeDirectionMessage(int8_t direction)
{
    scroll += 2;
    if (scroll >= sizeof(message) * FONT_WIDTH - 30) scroll = 0;
}

static void drawColumnMessage(uint8_t position)
{
    uint8_t v = 64;
    if (position < 6) v = 1 << position;
    if (position >= 24) v = 1 << (29 - position);
    uint32_t color = pixels.Color(v, v, v);

    int16_t adjustedPosition = position + scroll;
    uint8_t textPosition = adjustedPosition / FONT_WIDTH;
    uint8_t textColumn = adjustedPosition % FONT_WIDTH;
    uint8_t pattern = getFontPattern(pgm_read_byte(&message[textPosition]), textColumn);
    for (uint8_t i = 0; i < NUMBER_PIXELS; i++) {
        pixels.setPixelColor(i, bitRead(pattern, i) ? color : 0);
    }
    pixels.show();
}

/*---------------------------------------------------------------------------*/

static void initCounter(void)
{
    counter = 0;
    numerals[0] = '0';
    length = 1;
}

static void changeDirectionCounter(int8_t direction)
{
    if ((++counter & 1) == 0) {
        if (counter >= 20000) counter = 0;
        length = snprintf(numerals, sizeof(numerals), "%d", counter >> 1);
    }
}

static void drawColumnCounter(uint8_t position)
{
    int16_t adjustedPosition = position - (4 - length) * (FONT_WIDTH / 2);
    if (adjustedPosition < 0 || adjustedPosition >= length * FONT_WIDTH) {
        drawMargin();
        return;
    }
    uint8_t textPosition = adjustedPosition / FONT_WIDTH;
    uint8_t textColumn = adjustedPosition % FONT_WIDTH;
    uint8_t pattern = getFontPattern(numerals[textPosition], textColumn);
    uint16_t hue = (counter << 4) % HUE_MAX;
    for (uint8_t i = 0; i < NUMBER_PIXELS; i++) {
        pixels.setPixelColor(i, bitRead(pattern, i) ? hsv2Color(hue, 192 + i * 8, 64) : 0);
    }
    pixels.show();
}

/*---------------------------------------------------------------------------*/

static void initOBONO(void)
{
    counter = 0;
    prepareOBONO();
}

static void prepareOBONO(void)
{
    uint8_t hsv[6];
    if (counter < 3) {
        memcpy_P(hsv, &hsvTable[counter], sizeof(hsv));
    } else {
        randomSeed(millis());
        for (uint8_t i = 0; i < 6; i++) {
            hsv[i] = (i % 3 < 2) ? random(256) : random(64) + 192;
        }
    }
    palette[0] = pixels.Color(0, 16, 16);
    palette[1] = pixels.Color(0, 0, 0);
    for (uint8_t i = 0; i < 5; i++) {
        palette[2 + i] = hsv2Color(hsv[0] * 6, hsv[1], hsv[2] >> (5 - i));
        palette[7 + i] = hsv2Color(hsv[3] * 6, hsv[4], hsv[5] >> (5 - i));
    }
    scroll = 0;
}

static void changeDirectionOBONO(int8_t direction)
{
    if (++scroll > 22) {
        counter++;
        prepareOBONO();
    }
}

static void drawColumnOBONO(uint8_t position)
{
    uint8_t index = position >> 2;
    uint8_t shift = (position & 3) << 2;
    for (uint8_t i = 0; i < NUMBER_PIXELS; i++) {
        int8_t y = scroll + i - 7;
        uint8_t c = 0;
        if (y >= 0 && y < 16) {
            uint16_t pattern = pgm_read_word(&bitmap[y][index]);
            c = pattern >> shift & 15;
        }
        pixels.setPixelColor(i, palette[c]);
    }
    pixels.show();
}

static void drawMarginOBONO(void)
{
    pixels.fill(palette[0]);
    pixels.show();
}
