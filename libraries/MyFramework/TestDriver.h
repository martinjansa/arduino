#ifndef TEST_DRIVER__H
#define TEST_DRIVER__H

#include <arduino.h>

class TestException {
private:
  const char *m_file;
  int m_lineNumber;
  const char *m_expression;

private:
  TestException();
  TestException(const TestException &);

public:
  TestException(const char *file, int lineNumber, const char *expression): m_file(file), m_lineNumber(lineNumber), m_expression(expression) {}

  void Dump()
  {
    Serial.println(F("****"));
    Serial.print(m_file);
    Serial.print(F("("));
    Serial.print(m_lineNumber, DEC);
    Serial.print(F("): TEST_ASSERT("));
    Serial.print(m_expression);
    Serial.println(F(") failed.\n****"));
  }

};

class TestDriver;

typedef void (*TestCaseFn)(TestDriver &);

class TestDriver {
private:
  int m_ledPin;
  bool currentTestFailed;
  bool testsFailed;

public:

  TestDriver(int ledPin = -1): m_ledPin(ledPin), testsFailed(false) {}

  void begin()
  {
    if (m_ledPin != -1) {
      pinMode(m_ledPin, OUTPUT);
    }
  }
  void StartTests()
  {
    Serial.println(F("Starting the tests.")); 
  }

  void FinishTests()
  {
    Serial.print(F("End of the tests: ")); 
    if (testsFailed) {
      Serial.println(F("FAILED."));
    } else {
      Serial.println(F("SUCEEDED."));
    }
  }

  void RunTestCase(TestCaseFn testCaseFn, const char *testCaseName)
  {
    Serial.print(testCaseName);
    Serial.println(F(" begin"));

    currentTestFailed = false;
    
    // run the test case
    (*testCaseFn)(*this);

    Serial.print(testCaseName);
    Serial.println(F(" end"));

    if (currentTestFailed) {

      // long blink to report failure
      if (m_ledPin != -1) {
        digitalWrite(m_ledPin, HIGH);
        delay(2000);
        digitalWrite(m_ledPin, LOW);
        delay(250);
      }

      testsFailed = true;
    } else {

      // short blink to report OK result
      if (m_ledPin != -1) {
        digitalWrite(m_ledPin, HIGH);
        delay(100);
        digitalWrite(m_ledPin, LOW);
        delay(250);
      }
    }
  }

  void ReportCurrentTestFailure()
  {
    currentTestFailed = true;
  }

  void SignalTestsStatusViaLED()
  {
    if (m_ledPin != -1) {

      if (testsFailed) {

        // report failure by quick blinking
        digitalWrite(m_ledPin, HIGH);
        delay(50);
        digitalWrite(m_ledPin, LOW);
        delay(50);

      } else {

        // report success by 3 short blinks repeated every 5 seconds
        delay(5000);

        for (int i = 0; i < 3; i++) {

          digitalWrite(m_ledPin, HIGH);
          delay(250);
          digitalWrite(m_ledPin, LOW);
          delay(250);
        }
      }
    }
  }

};

#define DECLARE_TEST_CASE(testCaseName) void testCaseName(TestDriver &testDriver) 

#define TEST_ASSERT(exp) { if (!(exp)) { TestException exception(__FILE__, __LINE__, #exp); exception.Dump(); testDriver.ReportCurrentTestFailure(); } }

#define RUN_TEST_CASE(testDriver, testCaseName) testDriver.RunTestCase(testCaseName, #testCaseName)

#endif // TEST_DRIVER__H
