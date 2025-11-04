// --- Notas musicales ---
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_DS5 622
#define NOTE_D5  587
#define NOTE_CS5 554
#define NOTE_G3  196

// --- Pines ---
const uint8_t ledPins[] = {9, 10, 11, 12};
const uint8_t buttonPins[] = {2, 3, 4, 5};
#define SPEAKER_PIN 8

// --- Configuración de juego ---
#define MAX_GAME_LENGTH 100
const int gameTones[] = {NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G5};

uint8_t gameSequence[MAX_GAME_LENGTH];
uint8_t gameIndex = 0;   // Longitud actual de la secuencia
uint8_t userStep = 0;    // Paso actual del jugador

// --- Estados ---
enum GameState { WAIT_START, PLAY_SEQUENCE, WAIT_USER, GAME_OVER };
GameState state = WAIT_START;

// --- Temporizadores ---
unsigned long lastMillis = 0;
int seqPos = 0;

// --- Setup ---
void setup() {
  Serial.begin(115200);
  for (byte i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);

  randomSeed(analogRead(A3));

  Serial.println("READY");  // Señal para Flask
}

// --- Funciones auxiliares ---
void lightLedAndPlayTone(byte i, int duration) {
  Serial.println("LED_ON:" + String(i));  // Informar LED encendido
  digitalWrite(ledPins[i], HIGH);
  tone(SPEAKER_PIN, gameTones[i]);
  delay(duration);
  digitalWrite(ledPins[i], LOW);
  noTone(SPEAKER_PIN);
  Serial.println("LED_OFF:" + String(i)); // Informar LED apagado
}

byte readButtonNonBlocking() {
  for (byte i = 0; i < 4; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      delay(50); // debounce
      while (digitalRead(buttonPins[i]) == LOW) delay(10);
      Serial.println("BUTTON_PRESS:" + String(i));  // Informar botón presionado
      return i;
    }
  }
  return 255;
}

void playLevelUpSound() {
  tone(SPEAKER_PIN, NOTE_E4); delay(100);
  tone(SPEAKER_PIN, NOTE_G4); delay(100);
  tone(SPEAKER_PIN, NOTE_E5); delay(100);
  tone(SPEAKER_PIN, NOTE_C5); delay(150);
  noTone(SPEAKER_PIN);
}

void playGameOverSound() {
  tone(SPEAKER_PIN, NOTE_DS5); delay(200);
  tone(SPEAKER_PIN, NOTE_D5); delay(200);
  tone(SPEAKER_PIN, NOTE_CS5); delay(200);
  noTone(SPEAKER_PIN);
}

void resetGame() {
  gameIndex = 0;
  userStep = 0;
  seqPos = 0;
  state = WAIT_START;
}

// --- Lógica de juego ---
void addNextToSequence() {
  if (gameIndex < MAX_GAME_LENGTH) {
    gameSequence[gameIndex] = random(0, 4);
    gameIndex++;
  }
}

void playSequence() {
  for (int i = 0; i < gameIndex; i++) {
    lightLedAndPlayTone(gameSequence[i], 400);
    delay(200);
  }
  seqPos = 0;
  userStep = 0;
  state = WAIT_USER;
  Serial.println("WAIT_USER");
}

void handleUserInput() {
  byte pressed = readButtonNonBlocking();
  if (pressed != 255) {
    lightLedAndPlayTone(pressed, 200);

    if (pressed == gameSequence[userStep]) {
      userStep++;
      if (userStep == gameIndex) {
        Serial.println("LEVEL_UP:" + String(gameIndex));
        playLevelUpSound();
        delay(400);
        addNextToSequence();
        state = PLAY_SEQUENCE;
      }
    } else {
      Serial.println("GAME_OVER:" + String(gameIndex - 1));
      playGameOverSound();
      resetGame();
    }
  }
}

// --- Loop principal ---
void loop() {
  // Comando desde Flask
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "START" && state == WAIT_START) {
      addNextToSequence();
      seqPos = 0;
      lastMillis = millis();
      state = PLAY_SEQUENCE;
      Serial.println("SEQUENCE_START");
    }
  }

  // Lógica de estados
  if (state == PLAY_SEQUENCE) {
    playSequence();
  }
  else if (state == WAIT_USER) {
    handleUserInput();
  }
  // En WAIT_START o GAME_OVER no hace nada
}
