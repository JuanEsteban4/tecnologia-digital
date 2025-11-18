const int buzzerPin = 3; 
const unsigned long blinkInterval = 300;  // ms between LED toggles

// ====== VARIABLES ======
int targetBlinks = 0;
int blinkCount = 0;
bool blinking = false;
bool ledState = HIGH;
unsigned long previousMillis = 0;

void buzzerSetup() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, ledState);
}

void setBuzzerTimes(int times){
    if (blinking) return;
    targetBlinks = times << 1;
    blinkCount = 0;
    blinking = true;
}
// ====== TASK 2: HANDLE BLINKING ======
void handleBuzzer(unsigned long currentMillis) {
  if (blinking) {
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(buzzerPin, ledState);
      blinkCount++;

      if (blinkCount >= targetBlinks) {
        blinking = false;
        ledState = HIGH;
        digitalWrite(buzzerPin, HIGH);
      }
    }
  }
}
