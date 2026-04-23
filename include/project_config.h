#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

#include <Arduino.h>

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#define BUTTON 0 // GPIO0 is often used for a button on ESP32 boards

#define LED1 27
#define LED2 4

#define KEY1 34
#define SW1 35
#define RP1 33


#define TAG1 "1a61c8fc" // Con camma
#define TAG2 "8ae5bffc" // Senza camma

#endif // __PROJECT_CONFIG_H__
