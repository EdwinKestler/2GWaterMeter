#include "Watchdog.h"

#if defined(__AVR__)
#include <avr/wdt.h>
#include <avr/io.h>

void waterbox_wdt_off(void) __attribute__((naked, used, section(".init3")));
void waterbox_wdt_off(void) {
  MCUSR = 0;
  wdt_disable();
}

void Watchdog::enable() {
  wdt_enable(WDTO_8S);
}

void Watchdog::kick() {
  wdt_reset();
}

#else

void Watchdog::enable() {}

void Watchdog::kick() {}

#endif
