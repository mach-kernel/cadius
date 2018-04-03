#include "log.h"
#include <stdarg.h>

static struct logconf LCF = { true };

int logf(const char *fmt, ...)
{
  if (!LCF.enabled) return 0;

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