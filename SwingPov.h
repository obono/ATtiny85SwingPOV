#pragma once

#include <Arduino.h>

/*  Typedefs  */

typedef struct {
    uint8_t windowWidth;
    uint8_t marginWidth;
    void    (*init)(void);
    void    (*changeDirection)(int8_t);
    void    (*displayColumn)(uint8_t);
    void    (*displayMargin)(void);
} CONFIG_T;

/*  Global Functions  */

void        initSwingPov(uint8_t enablePin_, uint8_t statusPin_);
void        configSwingPov(CONFIG_T &config_);
void        updateSwingPov(void);
uint32_t    getNextTime(void);
