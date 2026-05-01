#include <Arduino.h>

// --- Pin ---
#define PHOTORESISTOR_PIN  3  // GPIO3 (ADC1_CH2 on ESP32-S3)

// --- Timer intervals (microseconds) ---
#define TIMER1_INTERVAL_US  1000000  // 1 second
#define TIMER2_INTERVAL_US  3000000  // 3 seconds

// --- Hardware timer pointers ---
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;

// --- Flags set by ISR, processed in loop ---
volatile bool timer1Fired = false;
volatile bool timer2Fired = false;

// --- Reading counters ---
int timer1Count = 0;
int timer2Count = 0;

// --- Timer ISR handlers ---
void IRAM_ATTR onTimer1() {
  timer1Fired = true;
}

void IRAM_ATTR onTimer2() {
  timer2Fired = true;
}

void readPhotoresistor(const char *timerName, int &count) {
  count++;
  int raw = analogRead(PHOTORESISTOR_PIN);
  float voltage = raw * (3.3 / 4095.0);

  Serial.printf("[%s] #%d | RAW: %4d | Voltage: %.2f V | %s\n",
    timerName, count, raw, voltage,
    raw > 3000 ? "Dark" : (raw > 1500 ? "Medium" : "Bright"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Lab 3.2: ESP32 Hardware Timers ===\n");
  Serial.println("Timer 1: reads every 1 second");
  Serial.println("Timer 2: reads every 3 seconds\n");

  // Timer 1: prescaler 80 → 1 tick = 1 µs (80 MHz / 80 = 1 MHz)
  timer1 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, TIMER1_INTERVAL_US, true);
  timerAlarmEnable(timer1);

  // Timer 2: same prescaler, different interval
  timer2 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  timerAlarmWrite(timer2, TIMER2_INTERVAL_US, true);
  timerAlarmEnable(timer2);
}

void loop() {
  if (timer1Fired) {
    timer1Fired = false;
    readPhotoresistor("Timer1 (1s)", timer1Count);
  }

  if (timer2Fired) {
    timer2Fired = false;
    readPhotoresistor("Timer2 (3s)", timer2Count);
  }

  delay(10);
}