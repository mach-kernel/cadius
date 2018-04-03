#pragma once

#include <stdbool.h>
  
typedef struct logconf {
  bool enabled;
} logconf;

int logf(const char *fmt, ...);
void log_off();
void log_on();