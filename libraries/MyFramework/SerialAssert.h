#ifndef SERIAL_ASSERT__H
#define SERIAL_ASSERT__H

//#define __ASSERT_USE_STDERR

#include <arduino.h>
#include <assert.h>

// handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
  // transmit diagnostic informations through serial link.
  Serial.println(F("***"));
  Serial.print(__file);
  Serial.print(F("("));
  Serial.print(__lineno, DEC);
  Serial.print(F("): "));
  Serial.print(__func);
  Serial.print(F(": ASSERT("));
  Serial.print(__sexp);
  Serial.println(F(") failed.\n***"));
  Serial.flush();
  // abort program execution.
  abort();
}

#endif // SERIAL_ASSERT__H
