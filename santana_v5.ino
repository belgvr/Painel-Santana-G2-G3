#include <FastLED.h>
#include <EEPROM.h>

// Pinos
#define PIN_numbers 6
#define PIN_needles 5
#define PIN_lcd 3

#define NUMPIXELS_numbers 16
#define NUMPIXELS_needles 8
#define NUMPIXELS_lcd 4

// Arrays de LEDs
CRGB numbers[NUMPIXELS_numbers];
CRGB needles[NUMPIXELS_needles];
CRGB lcd[NUMPIXELS_lcd];

// Botões (agora 4)
#define btn_one 2    // Seletor de cor / selecionar strip (long press entra/saida modo cor)
#define btn_three 4  // Down (brilho - por padrao / preset - no modo de selecao)
#define btn_four 7   // Up (brilho + por padrao / preset + no modo de selecao)
#define btn_effect 12 // Novo: controle de efeitos (long press entra/saida modo efeito)
#define BUZZER_PIN 11  // pino do buzzer

// ========== CONFIGURAÇÕES DE SOM ==========
// Frequências
const int FREQ_HIGH = 6400;   // Frequência aguda "ti"
const int FREQ_LOW = 3200;   // Frequência grave "tu"

// Durações
const int DURATION_SHORT = 80;    // Duração para sons simples (ti, tu)
const int DURATION_NORMAL = 120;  // Duração para sons compostos
const int DURATION_FAST = 60;     // Duração para sons rápidos (máximo/mínimo)
const int PAUSE_SHORT = 20;       // Pausa curta entre notas
const int PAUSE_NORMAL = 30;      // Pausa normal entre notas

// Estado de cor (presets)
int numbers_case = 0;
int needles_case = 0;
int lcd_case = 0;

// Estado brilho
int brightness_level = 5;

// Seletor de cor
int selected_strip = 0;  // 0=numbers, 1=needles, 2=lcd
bool selection_mode = false;   // Modo de selecao de cor (btn_one long)

// Modo de efeito
bool effect_mode = false;
int selected_effect_strip = 0; // para trocar strip dentro do modo efeito (btn_effect short)

bool resetMode = false;
unsigned long resetStartTime = 0;
unsigned long lastResetFlash = 0;
const unsigned long resetHoldTime = 5000; // 5 segundos

// Efeitos por strip (IDs)
enum Effects {
  EFFECT_NONE = 0,
  EFFECT_WAVE,
  EFFECT_RAINBOW_FAST,
  EFFECT_RAINBOW_SLOW,
  EFFECT_STROBE_FAST,
  EFFECT_STROBE_SLOW,
  EFFECT_FADE_SLOW,
  EFFECT_RGB_CYCLE_FAST,
  EFFECT_RGB_CYCLE_SLOW,
  NUM_EFFECTS
};

int numbers_effect = EFFECT_NONE;
int needles_effect = EFFECT_NONE;
int lcd_effect = EFFECT_NONE;

// Controle de debounce (4 botoes)
unsigned long lastDebounceTime[4] = {0,0,0,0};
bool lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
bool buttonState[4] = {HIGH, HIGH, HIGH, HIGH};
const unsigned long debounceDelay = 50;

// Controle de botão segurado
unsigned long buttonHeldTime[4] = {0,0,0,0};
const unsigned long holdThreshold = 500; // ms para considerar hold curto (repeticao)
const unsigned long longPressThreshold = 2000; // ms para long press (entrar/saida modo)
const unsigned long holdRepeatDelay = 120; // repetir ajuste enquanto segurado

// Temporizador e EEPROM save
unsigned long lastActionTime = 0;
const unsigned long selectionTimeout = 15000; // sai do modo de selecao apos 15s
unsigned long lastEEPROMSave = 0;

// Brilho levels
const uint8_t brightnessLevels[10] = {25,40,60,80,110,140,170,200,230,255};
const uint8_t min_brightness_level = 20; // brilho para tiras nao selecionadas no modo cor

// Paleta
const CRGB colorPalette[] = {
  CRGB(255, 0, 0),        // VERMELHO
  CRGB(0, 255, 0),        // VERDE
  CRGB(0, 0, 255),        // AZUL
  CRGB(0, 255, 255),      // CIANO
  CRGB(150, 0, 255),      // ROXO
  CRGB(255, 150, 0),      // AMARELO
  CRGB(200, 200, 0),      // LIME
  CRGB(255, 20, 0),        // NEON ORANGE
  CRGB(255, 0, 100),      // PINK
  CRGB(255, 255, 255),    // BRANCO
  CRGB(40, 40, 255),      // VWBLUE
  CRGB(30, 0, 180),     // VWBLUE2
  CRGB(0, 130, 255),      // SANTANABLUE
  CRGB(80, 255, 20),    // SANTANAGREEN
  CRGB(100, 0, 255),     // LIGHT UV
  CRGB(70, 0, 255),       // UV
  CRGB(255, 60, 20),    // SALMON
  CRGB(80, 150, 255),    // DEEPCYAN
};
const int numColors = sizeof(colorPalette) / sizeof(CRGB);

const char* colorNames[] = {
  "VERMELHO",
  "VERDE",
  "AZUL",
  "CIANO",
  "ROXO",
  "AMARELO",
  "LIME",
  "NEON ORANGE",
  "PINK",
  "BRANCO",
  "VWBLUE",
  "VWBLUE2",
  "SANTANABLUE",
  "SANTANAGREEN",
  "LIGHT UV",
  "UV",
  "SALMON",
  "DEEPCYAN"
};

// ========== FUNÇÕES DE SOM ==========
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

void soundEnterColorMode() {
  // "tu-ti" - entrada seleção cores
  playTone(FREQ_LOW, DURATION_NORMAL); // tu (baixo)
  delay(PAUSE_NORMAL);
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti (alto)
}

void soundChangeStripe() {
  // "ti-ti" - troca de strip
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti (alto)
  delay(PAUSE_NORMAL);
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti (alto)
}

void soundExitColorMode() {
  // "ti-tu" - saída seleção cores
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti (alto)
  delay(PAUSE_NORMAL);
  playTone(FREQ_LOW, DURATION_NORMAL); // tu (baixo)
}

void soundUp() {
  // "ti" - up
  playTone(FREQ_HIGH, DURATION_SHORT); // ti (alto curto)
}

void soundUpMax() {
  // "ti-ti-ti" - up máximo
  playTone(FREQ_HIGH, DURATION_FAST); // ti
  delay(PAUSE_SHORT);
  playTone(FREQ_HIGH, DURATION_FAST); // ti
  delay(PAUSE_SHORT);
  playTone(FREQ_HIGH, DURATION_FAST); // ti
}

void soundDown() {
  // "tu" - down
  playTone(FREQ_LOW, DURATION_SHORT); // tu (baixo curto)
}

void soundDownMin() {
  // "tu-tu-tu" - down mínimo
  playTone(FREQ_LOW, DURATION_FAST); // tu
  delay(PAUSE_SHORT);
  playTone(FREQ_LOW, DURATION_FAST); // tu
  delay(PAUSE_SHORT);
  playTone(FREQ_LOW, DURATION_FAST); // tu
}

void soundEnterEffectMode() {
  // "ti-tu-ti" - entrada modo efeito
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti
  delay(PAUSE_NORMAL);
  playTone(FREQ_LOW, DURATION_NORMAL); // tu
  delay(PAUSE_NORMAL);
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti
}

void soundExitEffectMode() {
  // "tu-ti-tu" - saída modo efeito
  playTone(FREQ_LOW, DURATION_NORMAL); // tu
  delay(PAUSE_NORMAL);
  playTone(FREQ_HIGH, DURATION_NORMAL); // ti
  delay(PAUSE_NORMAL);
  playTone(FREQ_LOW, DURATION_NORMAL); // tu
}

// Prototypes
void updateAllLEDs();
void flashStrip(int strip, CRGB color, int flashes);
void flashAllLEDs(CRGB color, int flashes);
void flashAllGreen();
void runEffectDown();
void runEffectUp();

bool buttonPressed(int idx, int pin);
bool buttonHeld(int idx, int pin);
bool buttonLongPress(int idx, int pin);

void processButtons();
void startupEffect();
void applyEffects(); // non-blocking, aplica efeitos se houver
void applyEffectToStrip(CRGB *leds, int numLeds, int effect, int colorCase, uint8_t &hueRef);

// Hues para cada strip (estado rotativo)
uint8_t hueNumbers = 0;
uint8_t hueNeedles = 64;
uint8_t hueLcd = 128;

void setup() {
  FastLED.addLeds<WS2812B, PIN_numbers, GRB>(numbers, NUMPIXELS_numbers);
  FastLED.addLeds<WS2812B, PIN_needles, GRB>(needles, NUMPIXELS_needles);
  FastLED.addLeds<WS2812B, PIN_lcd, GRB>(lcd, NUMPIXELS_lcd);

  pinMode(btn_one, INPUT_PULLUP);
  pinMode(btn_three, INPUT_PULLUP);
  pinMode(btn_four, INPUT_PULLUP);
  pinMode(btn_effect, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("=== SISTEMA PRONTO ===");
  Serial.println("1=SeletorCor | 3=Down | 4=Up | 12=Efeito");
  Serial.println("==================");

  // Load values from EEPROM (enderecos: 0,4,8,12 para cores/brilho; 16,20,24 para efeitos)
  EEPROM.get(0, numbers_case);
  EEPROM.get(4, needles_case);
  EEPROM.get(8, lcd_case);
  EEPROM.get(12, brightness_level);

  EEPROM.get(16, numbers_effect);
  EEPROM.get(20, needles_effect);
  EEPROM.get(24, lcd_effect);

  // Sanity
  numbers_case = constrain(numbers_case, 0, numColors - 1);
  needles_case = constrain(needles_case, 0, numColors - 1);
  lcd_case = constrain(lcd_case, 0, numColors - 1);
  brightness_level = constrain(brightness_level, 1, 10);

  numbers_effect = constrain(numbers_effect, 0, NUM_EFFECTS-1);
  needles_effect = constrain(needles_effect, 0, NUM_EFFECTS-1);
  lcd_effect = constrain(lcd_effect, 0, NUM_EFFECTS-1);

  // Startup
  startupEffect();

  FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
  updateAllLEDs();
  FastLED.show();
  
}

void loop() {
  processButtons();

  // Aplica efeitos se houver (não-bloqueante)
  applyEffects();

  // Salva no EEPROM se houver alterações recentes (a cada 5s desde ultima acao)
  if (millis() - lastActionTime > 5000 && lastActionTime != 0 && millis() - lastEEPROMSave > 5000) {
    EEPROM.put(0, numbers_case);
    EEPROM.put(4, needles_case);
    EEPROM.put(8, lcd_case);
    EEPROM.put(12, brightness_level);
    EEPROM.put(16, numbers_effect);
    EEPROM.put(20, needles_effect);
    EEPROM.put(24, lcd_effect);
    lastEEPROMSave = millis();
    Serial.println("Valores salvos no EEPROM.");
  }

  // Timeout modo selecao cor
  if (selection_mode) {
      if (millis() - lastActionTime > selectionTimeout) {
          Serial.println("Tempo esgotado. Saindo do Modo de Selecao de Cor.");
          selection_mode = false;
          soundExitColorMode(); // SOM: saída automática do modo cor
          FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
          flashAllGreen();
          updateAllLEDs();
          FastLED.show();
      }
  }

  // Timeout modo efeito
  if (effect_mode) {
      if (millis() - lastActionTime > selectionTimeout) {
          Serial.println("Tempo esgotado. Saindo do Modo de Selecao de Efeito.");
          effect_mode = false;
          soundExitEffectMode(); // SOM: saída automática do modo efeito
          FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
          flashAllGreen();
          updateAllLEDs();
          FastLED.show();
      }
  }

  delay(10);
}

void updateAllLEDs() {
    FastLED.clear();
    // No modo de seleção, mostra a cor selecionada e as outras com brilho mínimo
    if (selection_mode) {
        fill_solid(numbers, NUMPIXELS_numbers, colorPalette[numbers_case]);
        fill_solid(needles, NUMPIXELS_needles, colorPalette[needles_case]);
        fill_solid(lcd, NUMPIXELS_lcd, colorPalette[lcd_case]);

        if (selected_strip == 0) {
            fadeLightBy(needles, NUMPIXELS_needles, 255 - min_brightness_level);
            fadeLightBy(lcd, NUMPIXELS_lcd, 255 - min_brightness_level);
        } else if (selected_strip == 1) {
            fadeLightBy(numbers, NUMPIXELS_numbers, 255 - min_brightness_level);
            fadeLightBy(lcd, NUMPIXELS_lcd, 255 - min_brightness_level);
        } else {
            fadeLightBy(numbers, NUMPIXELS_numbers, 255 - min_brightness_level);
            fadeLightBy(needles, NUMPIXELS_needles, 255 - min_brightness_level);
        }
    } else {
        // Modo normal, todas as tiras na cor e brilho salvos (se não há efeito)
        fill_solid(numbers, NUMPIXELS_numbers, colorPalette[numbers_case]);
        fill_solid(needles, NUMPIXELS_needles, colorPalette[needles_case]);
        fill_solid(lcd, NUMPIXELS_lcd, colorPalette[lcd_case]);
    }
}

// ---------- Efeitos (non-blocking) ----------
void applyEffects() {
  bool anyEffect = (numbers_effect != EFFECT_NONE) || (needles_effect != EFFECT_NONE) || (lcd_effect != EFFECT_NONE);

  if (!anyEffect) {
    // Nenhum efeito ativo: manter cores estáticas (manual)
    updateAllLEDs();
    FastLED.show();
    return;
  }

  // Se algum efeito ativo, preparar cada strip
  if (numbers_effect == EFFECT_NONE) {
    fill_solid(numbers, NUMPIXELS_numbers, colorPalette[numbers_case]);
  } else {
    applyEffectToStrip(numbers, NUMPIXELS_numbers, numbers_effect, numbers_case, hueNumbers);
  }

  if (needles_effect == EFFECT_NONE) {
    fill_solid(needles, NUMPIXELS_needles, colorPalette[needles_case]);
  } else {
    applyEffectToStrip(needles, NUMPIXELS_needles, needles_effect, needles_case, hueNeedles);
  }

  if (lcd_effect == EFFECT_NONE) {
    fill_solid(lcd, NUMPIXELS_lcd, colorPalette[lcd_case]);
  } else {
    applyEffectToStrip(lcd, NUMPIXELS_lcd, lcd_effect, lcd_case, hueLcd);
  }

  FastLED.show();
}

// Helper que aplica um efeito numa strip usando a cor "base" index colorCase
void applyEffectToStrip(CRGB *leds, int numLeds, int effect, int colorCase, uint8_t &hueRef) {
  uint32_t t = millis();

  CRGB base = colorPalette[ constrain(colorCase, 0, numColors-1) ];

  switch (effect) {
    case EFFECT_WAVE: {
      // Ondinha: sin8 para cada led, fase por posicao. velocidade moderada.
      // amplitude entre 60..255
      for (int i=0;i<numLeds;i++) {
        uint8_t phase = i * (256 / max(1, numLeds)) + (t / 8); // desloca com tempo
        uint8_t wave = sin8(phase); // 0..255
        uint8_t bright = qadd8(60, (wave / 1)); // 60..315 clamp -> nscale
        CRGB c = base;
        c.nscale8_video(bright);
        leds[i] = c;
      }
      break;
    }

    case EFFECT_RAINBOW_FAST: {
      // Usa fill_rainbow com passo alto (rápido)
      fill_rainbow(leds, numLeds, hueRef, 8);
      hueRef += 6; // rápido
      break;
    }

    case EFFECT_RAINBOW_SLOW: {
      fill_rainbow(leds, numLeds, hueRef, 6);
      hueRef += 1; // lento
      break;
    }

    case EFFECT_STROBE_FAST: {
      // Liga/desliga rápido (100ms)
      uint8_t on = ((t / 100) & 1);
      if (on) fill_solid(leds, numLeds, CRGB::White);
      else FastLED.clear();
      break;
    }

    case EFFECT_STROBE_SLOW: {
      // Liga/desliga lento (400ms)
      uint8_t on = ((t / 400) & 1);
      if (on) fill_solid(leds, numLeds, CRGB::White);
      else FastLED.clear();
      break;
    }

    case EFFECT_FADE_SLOW: {
      // Fade in/out suave: ciclo ~10s (10000ms)
      // sin8 range 0..255 -> use to scale brightness 30..255
      uint8_t s = sin8(t / 40); // ~6.4s period for sin8 increments -> good smooth
      uint8_t bright = map(s, 0, 255, 40, 255);
      for (int i=0;i<numLeds;i++) {
        CRGB c = base;
        c.nscale8_video(bright);
        leds[i] = c;
      }
      break;
    }

    case EFFECT_RGB_CYCLE_FAST: {
      // Ciclo rapido entre hues (10s cycle-ish)
      fill_solid(leds, numLeds, CHSV(hueRef, 255, 255));
      hueRef += 3;
      break;
    }

    case EFFECT_RGB_CYCLE_SLOW: {
      // Ciclo lento (20s)
      fill_solid(leds, numLeds, CHSV(hueRef, 255, 255));
      hueRef += 1;
      break;
    }

    default:
      // fallback: pinta com base
      fill_solid(leds, numLeds, base);
      break;
  }
}

// ---------- Botões ----------

bool buttonPressed(int idx, int pin) {
  bool reading = digitalRead(pin);
  if (reading != lastButtonState[idx]) lastDebounceTime[idx] = millis();

  if ((millis() - lastDebounceTime[idx]) > debounceDelay) {
    if (reading != buttonState[idx]) {
      buttonState[idx] = reading;
      if (buttonState[idx] == LOW) {
        lastButtonState[idx] = reading;
        buttonHeldTime[idx] = millis();
        return true;
      }
    }
  }

  lastButtonState[idx] = reading;
  return false;
}

bool buttonHeld(int idx, int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - buttonHeldTime[idx] > holdThreshold) {
      if (millis() - lastDebounceTime[idx] > holdRepeatDelay) {
        lastDebounceTime[idx] = millis();
        return true;
      }
    }
  }
  return false;
}

bool buttonLongPress(int idx, int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - buttonHeldTime[idx] > longPressThreshold) {
      if (buttonState[idx] == LOW) {
        // Evita que dispare varias vezes
        buttonState[idx] = HIGH;
        return true;
      }
    }
  } else {
    // Reset time on release
    buttonHeldTime[idx] = millis();
  }
  return false;
}

void processButtons() {
 // Verificar reset (botões 3 e 4 juntos) - PRIMEIRA PRIORIDADE
 bool btn3Down = (digitalRead(btn_three) == LOW);
 bool btn4Down = (digitalRead(btn_four) == LOW);

 if (btn3Down && btn4Down && !resetMode) {
   resetStartTime = millis();
   resetMode = true;
   lastResetFlash = 0;
   Serial.println("Iniciando reset - mantenha pressionado...");
   return;
 } else if (btn3Down && btn4Down && resetMode) {
   if (millis() - resetStartTime >= resetHoldTime) {
     flashOrangeReset();
   }
   return;
 } else if (resetMode && (!btn3Down || !btn4Down)) {
   if (millis() - resetStartTime >= resetHoldTime) {
     performReset();
   } else {
     resetMode = false;
     Serial.println("Reset cancelado.");
   }
   return;
 }

 // BTN 1: Seletor de cor (btn_one)
 if (buttonLongPress(0, btn_one)) {
     selection_mode = !selection_mode;
     lastActionTime = millis();
     if (selection_mode) {
         Serial.println("Entrando no Modo de Selecao de Cor.");
         soundEnterColorMode();
         FastLED.setBrightness(255);
         if (selected_strip == 0) Serial.println("Strip selecionada: NUMEROS");
         else if (selected_strip == 1) Serial.println("Strip selecionada: AGULHAS");
         else Serial.println("Strip selecionada: LCDS");
         flashStrip(selected_strip, CRGB::White, 2);
     } else {
         Serial.println("Saindo do Modo de Selecao de Cor.");
         soundExitColorMode();
         saveToEEPROM();
         FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
         flashAllGreen();
     }
     updateAllLEDs();
     FastLED.show();
 } else if (buttonPressed(0, btn_one)) {
   if (selection_mode) {
       selected_strip = (selected_strip + 1) % 3;
       soundChangeStripe();
       lastActionTime = millis();
       Serial.print("Strip selecionada: ");
       if (selected_strip == 0) Serial.println("NUMEROS");
       else if (selected_strip == 1) Serial.println("AGULHAS");
       else Serial.println("LCDS");
       flashStrip(selected_strip, CRGB::White, 2);
       updateAllLEDs();
       FastLED.show();
   }
 }

 // BTN 3: Down - SÓ PROCESSA SE NÃO ESTIVER EM RESET
 if ((buttonPressed(1, btn_three) || buttonHeld(1, btn_three)) && !resetMode) {
   if (!selection_mode && !effect_mode) {
     // Brilho normal
     if (brightness_level > 1) {
       brightness_level--;
       soundDown();
       saveToEEPROM();
       FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
       Serial.print("Brilho: "); Serial.println(brightness_level);
       lastActionTime = millis();
       FastLED.show();
     } else {
       soundDownMin();
       runEffectDown();
     }
   } else if (selection_mode) {
     // muda preset de cor
     soundDown();
     lastActionTime = millis();
     if (selected_strip == 0) {
       numbers_case = (numbers_case - 1 + numColors) % numColors;
       Serial.print("Numeros preset: "); Serial.println(numbers_case);Serial.print(" - "); 
Serial.println(colorNames[numbers_case]);
     } else if (selected_strip == 1) {
       needles_case = (needles_case - 1 + numColors) % numColors;
       Serial.print("Agulhas preset: "); Serial.println(needles_case);Serial.print(" - "); 
Serial.println(colorNames[needles_case]);
     } else {
       lcd_case = (lcd_case - 1 + numColors) % numColors;
       Serial.print("LCDs preset: "); Serial.println(lcd_case);Serial.print(" - "); 
Serial.println(colorNames[lcd_case]);
     }
     updateAllLEDs();
     FastLED.show();
   } else if (effect_mode) {
     // muda efeito para a strip selecionada
     soundDown();
     lastActionTime = millis();
     if (selected_effect_strip == 0) {
       numbers_effect = (numbers_effect - 1 + NUM_EFFECTS) % NUM_EFFECTS;
       Serial.print("Numbers effect: "); Serial.println(numbers_effect);
     } else if (selected_effect_strip == 1) {
       needles_effect = (needles_effect - 1 + NUM_EFFECTS) % NUM_EFFECTS;
       Serial.print("Needles effect: "); Serial.println(needles_effect);
     } else {
       lcd_effect = (lcd_effect - 1 + NUM_EFFECTS) % NUM_EFFECTS;
       Serial.print("LCD effect: "); Serial.println(lcd_effect);
     }
     updateAllLEDs();
     FastLED.show();
   }
 }

 // BTN 4: Up - SÓ PROCESSA SE NÃO ESTIVER EM RESET
 if ((buttonPressed(2, btn_four) || buttonHeld(2, btn_four)) && !resetMode) {
   if (!selection_mode && !effect_mode) {
     // Brilho normal
     if (brightness_level < 10) {
       brightness_level++;
       soundUp();
       saveToEEPROM();
       FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
       Serial.print("Brilho: "); Serial.println(brightness_level);
       lastActionTime = millis();
       FastLED.show();
     } else {
       soundUpMax();
       runEffectUp();
     }
   } else if (selection_mode) {
     // muda preset de cor
     soundUp();
     lastActionTime = millis();
     if (selected_strip == 0) {
       numbers_case = (numbers_case + 1) % numColors;
       Serial.print("Numeros preset: "); Serial.println(numbers_case);Serial.print(" - "); 
Serial.println(colorNames[numbers_case]);
       
     } else if (selected_strip == 1) {
       needles_case = (needles_case + 1) % numColors;
       Serial.print("Agulhas preset: "); Serial.println(needles_case);Serial.print(" - "); 
Serial.println(colorNames[needles_case]);
     } else {
       lcd_case = (lcd_case + 1) % numColors;
       Serial.print("LCDs preset: "); Serial.println(lcd_case);Serial.print(" - "); 
Serial.println(colorNames[lcd_case]);
     }
     updateAllLEDs();
     FastLED.show();
   } else if (effect_mode) {
     // muda efeito para a strip selecionada
     soundUp();
     lastActionTime = millis();
     if (selected_effect_strip == 0) {
       numbers_effect = (numbers_effect + 1) % NUM_EFFECTS;
       Serial.print("Numbers effect: "); Serial.println(numbers_effect);
     } else if (selected_effect_strip == 1) {
       needles_effect = (needles_effect + 1) % NUM_EFFECTS;
       Serial.print("Needles effect: "); Serial.println(needles_effect);
     } else {
       lcd_effect = (lcd_effect + 1) % NUM_EFFECTS;
       Serial.print("LCD effect: "); Serial.println(lcd_effect);
     }
     updateAllLEDs();
     FastLED.show();
   }
 }

 // BTN EFFECT: entra/sai modo efeito (long press) ou troca strip no modo efeito (short press)
 if (buttonLongPress(3, btn_effect)) {
     effect_mode = !effect_mode;
     lastActionTime = millis();
     if (effect_mode) {
         Serial.println("Entrando no Modo de Selecao de Efeito.");
         soundEnterEffectMode();
         FastLED.setBrightness(255);
         if (selected_effect_strip == 0) Serial.println("Editar: NUMEROS");
         else if (selected_effect_strip == 1) Serial.println("Editar: AGULHAS");
         else Serial.println("Editar: LCDS");
         flashStrip(selected_effect_strip, CRGB::White, 2);
     } else {
         Serial.println("Saindo do Modo de Selecao de Efeito.");
         soundExitEffectMode();
         saveToEEPROM();
         FastLED.setBrightness(brightnessLevels[brightness_level - 1]);
         flashAllGreen();
     }
     updateAllLEDs();
     FastLED.show();
 } else if (buttonPressed(3, btn_effect)) {
   if (effect_mode) {
     selected_effect_strip = (selected_effect_strip + 1) % 3;
     lastActionTime = millis();
     Serial.print("Editar strip: ");
     if (selected_effect_strip == 0) Serial.println("NUMEROS");
     else if (selected_effect_strip == 1) Serial.println("AGULHAS");
     else Serial.println("LCDS");
     flashStrip(selected_effect_strip, CRGB::White, 2);
     updateAllLEDs();
     FastLED.show();
   }
 }
}

// ---------- Utilitários e efeitos antigos ----------
void startupEffect() {
  const int duration = 800; // ms
  const int steps = 40;
  const int delayPerStep = duration / steps;

  CRGB targetNumbersColor = colorPalette[numbers_case];
  CRGB targetNeedlesColor = colorPalette[needles_case];
  CRGB targetLcdColor = colorPalette[lcd_case];
  uint8_t targetBrightness = brightnessLevels[brightness_level - 1];

  for (int i = 0; i <= steps; i++) {
    float progress = (float)i / steps;
    CRGB currentNumbersColor = blend(CRGB::Black, targetNumbersColor, (uint8_t)(progress * 255));
    CRGB currentNeedlesColor = blend(CRGB::Black, targetNeedlesColor, (uint8_t)(progress * 255));
    CRGB currentLcdColor = blend(CRGB::Black, targetLcdColor, (uint8_t)(progress * 255));

    fill_solid(numbers, NUMPIXELS_numbers, currentNumbersColor);
    fill_solid(needles, NUMPIXELS_needles, currentNeedlesColor);
    fill_solid(lcd, NUMPIXELS_lcd, currentLcdColor);

    FastLED.setBrightness((uint8_t)(progress * targetBrightness));
    FastLED.show();
    delay(delayPerStep);
  }
}

void flashStrip(int strip, CRGB color, int flashes) {
  for (int i = 0; i < flashes; i++) {
    if (strip == 0) fill_solid(numbers, NUMPIXELS_numbers, color);
    else if (strip == 1) fill_solid(needles, NUMPIXELS_needles, color);
    else fill_solid(lcd, NUMPIXELS_lcd, color);
    FastLED.show();

    int delayTime = 120;
    delay(delayTime);

    if (strip == 0) fill_solid(numbers, NUMPIXELS_numbers, CRGB::Black);
    else if (strip == 1) fill_solid(needles, NUMPIXELS_needles, CRGB::Black);
    else fill_solid(lcd, NUMPIXELS_lcd, CRGB::Black);
    FastLED.show();
    delay(delayTime);
  }
  updateAllLEDs();
  FastLED.show();
}

void flashAllLEDs(CRGB color, int flashes) {
  for (int i = 0; i < flashes; i++) {
    fill_solid(numbers, NUMPIXELS_numbers, color);
    fill_solid(needles, NUMPIXELS_needles, color);
    fill_solid(lcd, NUMPIXELS_lcd, color);
    FastLED.show();
    delay(200);

    FastLED.clear();
    FastLED.show();
    delay(200);
  }
}

void flashAllGreen() {
    for (int i = 0; i < 5; i++) {
        fill_solid(numbers, NUMPIXELS_numbers, CRGB(0, 255, 0));
        fill_solid(needles, NUMPIXELS_needles, CRGB(0, 255, 0));
        fill_solid(lcd, NUMPIXELS_lcd, CRGB(0, 255, 0));
        FastLED.show();
        delay(50);

        FastLED.clear();
        FastLED.show();
        delay(50);
    }
}

void runEffectDown() {
  int totalTime = 400;
  int delayTime = totalTime / max(1, NUMPIXELS_numbers);
  //for (int i = NUMPIXELS_numbers - 1; i >= 0; i--) {
  for (int i = NUMPIXELS_numbers - 1; i >= 0; i--) {
    FastLED.clear();
    numbers[i] = CRGB::Red;
    FastLED.show();
    delay(delayTime);
  }
  FastLED.clear();
  updateAllLEDs();
  FastLED.show();
}

void runEffectUp() {
  int totalTime = 400;
  int delayTime = totalTime / max(1, NUMPIXELS_numbers);
  //for (int i = 0; i < NUMPIXELS_numbers; i++) {
  for (int i = NUMPIXELS_numbers - 1; i >= 0; i--) {
    FastLED.clear();
    numbers[i] = CRGB::Green;
    FastLED.show();
    delay(delayTime);
  }
  FastLED.clear();
  updateAllLEDs();
  FastLED.show();
}

void performReset() {
  // Reset todos os valores para padrão
  numbers_case = 0;
  needles_case = 0;
  lcd_case = 0;
  brightness_level = 5;
  numbers_effect = EFFECT_NONE;
  needles_effect = EFFECT_NONE;
  lcd_effect = EFFECT_NONE;
  
  // Salva no EEPROM
  saveToEEPROM();
  
  // Reset via software
  asm volatile ("  jmp 0");
}

void flashOrangeReset() {
  if (millis() - lastResetFlash >= 500) {
    fill_solid(numbers, NUMPIXELS_numbers, CRGB(255, 179, 0)); // laranja
    fill_solid(needles, NUMPIXELS_needles, CRGB(255, 179, 0));
    fill_solid(lcd, NUMPIXELS_lcd, CRGB(255, 179, 0));
    FastLED.show();
    delay(100);
    
    FastLED.clear();
    FastLED.show();
    
    lastResetFlash = millis();
  }
}

void saveToEEPROM() {
  EEPROM.put(0, numbers_case);
  EEPROM.put(4, needles_case);
  EEPROM.put(8, lcd_case);
  EEPROM.put(12, brightness_level);
  EEPROM.put(16, numbers_effect);
  EEPROM.put(20, needles_effect);
  EEPROM.put(24, lcd_effect);
  Serial.println("Valores salvos no EEPROM.");
}
