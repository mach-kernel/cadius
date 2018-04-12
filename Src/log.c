#include "log.h"
#include <stdarg.h>

static struct logconf LCF = { 
  true,
  INFO
};

int logf_impl(loglevel level, const char *fmt, ...)
{
  if (!LCF.enabled || level > LCF.level) return 0;

  va_list varargs;
  va_start(varargs, fmt);

  return vprintf(fmt, varargs);
}

void log_off()
{
  LCF.enabled = false;
}

void log_on()
{
  LCF.enabled = true;
}

void log_set_level(loglevel level) 
{
  LCF.level = level;
}