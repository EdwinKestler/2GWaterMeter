#ifndef WATERBOX_WATCHDOG_H
#define WATERBOX_WATCHDOG_H

class Watchdog {
 public:
  static void enable();
  static void kick();
};

#endif
