// Monocon Tools end-to-end hardware validation.
// Uses only the Mega/ADK built-in LED and USB serial bridge.

constexpr unsigned long kHeartbeatIntervalMs = 250;
unsigned long previousHeartbeatMs = 0;
bool ledState = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.println("MONOCON_HW_OK");
}

void loop() {
  const unsigned long now = millis();
  if (now - previousHeartbeatMs < kHeartbeatIntervalMs) {
    return;
  }
  previousHeartbeatMs = now;
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  Serial.println("MONOCON_HW_OK");
}
