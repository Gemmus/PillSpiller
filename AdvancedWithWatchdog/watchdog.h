#include "pico.h"
#include "hardware/watchdog.h"

void watchdogInit(uint32_t timeout_ms);
void watchdogFeed();