/**
 * Basic logger
 * Author: David Stancu, @mach-kernel, Apr. 2018
 */

#pragma once
#include <stdbool.h>

typedef enum loglevel {
  ALWAYS,
  ERROR,
  INFO
} loglevel;

typedef struct logconf {
  bool enabled;
  loglevel level;
} logconf;

int logf_impl(loglevel level, const char *fmt, ...);

#define logf(fmt, ...) logf_impl(ALWAYS, fmt, ##__VA_ARGS__)
#define logf_info(fmt, ...) logf_impl(INFO, fmt, ##__VA_ARGS__)
#define logf_error(fmt, ...) logf_impl(ERROR, fmt, ##__VA_ARGS__)

void log_off();
void log_on();
void log_set_level(loglevel level);