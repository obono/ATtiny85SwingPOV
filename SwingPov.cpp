#include "SwingPov.h"

/*  Defines  */

#define SWING_TIME_MIN          150
#define SWING_TIME_MAX          250
#define SWING_TIME_AVE          ((SWING_TIME_MIN + SWING_TIME_MAX) / 2)
#define SWING_TIME_HISTORIES    16
#define SWING_PHASE_ADJUST      38

/*  Local Functions  */

static void initSwingTimeHistory(void);
static void tuneSwingTime(uint8_t swingTime);
static void controlDirection(uint8_t currentTiltState);

/*  Local Variables  */

static uint8_t  enablePin, statusPin;
static CONFIG_T config;

static uint8_t  totalWidth, lastTiltState;
static uint32_t baseTime, targetTime;
static int8_t   direction, position, positionMin, positionMax;
static uint16_t updateCounter;
static bool     isInitialzed = false, isTiltChanged;

static uint16_t swingTimeHistory[SWING_TIME_HISTORIES];
static uint8_t  swingTimeHistoryIndex;
static uint16_t swingTimeHistorySum, adjustedSwingTime;

/*---------------------------------------------------------------------------*/

void initSwingPov(uint8_t enablePin_, uint8_t statusPin_)
{
    enablePin = enablePin_;
    statusPin = statusPin_;
    pinMode(enablePin, OUTPUT);
    digitalWrite(enablePin, HIGH);
    pinMode(statusPin, INPUT);
    isInitialzed = true;
}

void configSwingPov(CONFIG_T &config_)
{
    if (!isInitialzed) return;
    config = config_;
    totalWidth = config.windowWidth + config.marginWidth * 2;
    positionMin = -config.marginWidth;
    positionMax = positionMin + totalWidth - 1;
    if (config.init) config.init();

    initSwingTimeHistory();
    baseTime = millis();
    targetTime = millis();
    uint8_t currentTiltState = digitalRead(statusPin);
    lastTiltState = !currentTiltState;
    controlDirection(currentTiltState);
}

void updateSwingPov(void)
{
    if (!isInitialzed) return;
    controlDirection(digitalRead(statusPin));
    if (position >= 0 && position < config.windowWidth) {
        if (config.displayColumn) config.displayColumn(position);
    } else {
        if (config.displayMargin) config.displayMargin();
    }
    position += direction;
    position = constrain(position, positionMin, positionMax);
    updateCounter++;
}

uint32_t getNextTime(void)
{
    if (!isInitialzed) return -1;
    return baseTime + adjustedSwingTime * updateCounter / totalWidth;
}

/*---------------------------------------------------------------------------*/

static void initSwingTimeHistory(void)
{
    for (uint8_t i = 0; i < SWING_TIME_HISTORIES; i++) {
        swingTimeHistory[i] = SWING_TIME_AVE;
    }
    swingTimeHistoryIndex = 0;
    swingTimeHistorySum = SWING_TIME_AVE * SWING_TIME_HISTORIES;
    adjustedSwingTime = SWING_TIME_AVE;
}

static void tuneSwingTime(uint8_t swingTime)
{
    if (swingTime >= SWING_TIME_MIN && swingTime <= SWING_TIME_MAX) {
        swingTimeHistorySum -= swingTimeHistory[swingTimeHistoryIndex];
        swingTimeHistorySum += swingTime;
        adjustedSwingTime = (swingTimeHistorySum + SWING_TIME_HISTORIES / 2) / SWING_TIME_HISTORIES;
        swingTimeHistory[swingTimeHistoryIndex] = swingTime;
        swingTimeHistoryIndex = (swingTimeHistoryIndex + 1) % SWING_TIME_HISTORIES;
    }
}

static void controlDirection(uint8_t currentTiltState)
{
    uint32_t currentTime = millis();
    if (lastTiltState != currentTiltState) {
        lastTiltState = currentTiltState;
        tuneSwingTime(currentTime - baseTime);
        baseTime = currentTime;
        updateCounter = 0;
        targetTime = currentTime + adjustedSwingTime * SWING_PHASE_ADJUST / 100;
        isTiltChanged = true;
    }
    if (isTiltChanged && currentTime >= targetTime) {
        if (currentTiltState == HIGH) {
            direction = -1;
            position = positionMax;
        } else {
            direction = 1;
            position = positionMin;
        }
        if (config.changeDirection) config.changeDirection(direction);
        isTiltChanged = false;
    }
}
