// ============================================================
//  AdrianAssistant — Main Entry Point & State Machine
// ============================================================
#include "config.h"
#include "oled.h"
#include "audio_io.h"
#include "stt.h"
#include "tts.h"
#include "ai_brain.h"
#include <WiFi.h>

enum AdrianState {
  STATE_IDLE,          
  STATE_LISTENING,     
  STATE_PROCESSING,    
  STATE_THINKING,      
  STATE_SPEAKING,      
  STATE_ERROR          
};

AdrianState adrianState = STATE_IDLE;
String        transcribedText = "";
AiResponse    aiResp;
unsigned long stateEnteredAt = 0;
extern bool   _ttsDone;

void audio_info(const char *info) { Serial.print("[Audio info] "); Serial.println(info); }
void audio_error(const char *info) { Serial.print("[Audio error] "); Serial.println(info); _ttsDone = true; }
void audio_eof_mp3(const char *info) { Serial.print("[TTS] EOF: "); Serial.println(info); _ttsDone = true; }

void enterState(AdrianState next) {
  adrianState = next;
  stateEnteredAt = millis();

  switch (next) {
    case STATE_IDLE:
      oledEmotion("idle");
      Serial.println("[STATE] IDLE — awaiting sensor trigger");
      break;
    case STATE_LISTENING:
      oledEmotion("listening");
      Serial.println("[STATE] LISTENING — recording raw audio stream...");
      startRecording();
      break;
    case STATE_PROCESSING:
      oledEmotion("thinking");
      Serial.println("[STATE] PROCESSING — contacting Wit.ai API endpoint...");
      break;
    case STATE_THINKING:
      oledEmotion("thinking");
      Serial.println("[STATE] THINKING — waiting for Adrian's brain core...");
      break;
    case STATE_SPEAKING:
      oledEmotion(aiResp.emotion);
      Serial.println("[STATE] SPEAKING — rendering text-to-speech stream...");
      break;
    case STATE_ERROR:
      oledEmotion("confused");
      Serial.println("[STATE] ERROR — triggering safety delay reset.");
      break;
  }
}

void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] Connection timed out!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== AdrianAssistant Booting ===");

  oledInit();
  oledEmotion("idle");
  connectWiFi();
  audioInit();

  pinMode(TOUCH_SWITCH_PIN, INPUT_PULLDOWN);

  Serial.println("=== Adrian Ready ===");
  enterState(STATE_IDLE);
}

void loop() {
  oledTick();
  audioTick();

  switch (adrianState) {
    case STATE_IDLE:
      if (digitalRead(TOUCH_SWITCH_PIN) == HIGH) {
        delay(50); 
        if (digitalRead(TOUCH_SWITCH_PIN) == HIGH) {
          playActivationBeep();
          enterState(STATE_LISTENING);
        }
      }
      break;

    case STATE_LISTENING:
      if (recordingDone()) {
        stopRecording();
        enterState(STATE_PROCESSING);
      }
      break;

    case STATE_PROCESSING: {
      transcribedText = runSTT();
      Serial.println("[STT] Text Transcript: " + transcribedText);
      if (transcribedText.length() > 0) {
        enterState(STATE_THINKING);
      } else {
        enterState(STATE_ERROR);
      }
      break;
    }

    case STATE_THINKING: {
      aiResp = askAI(transcribedText);
      Serial.println("[Adrian] Response parsed: " + aiResp.text_response);
      enterState(STATE_SPEAKING);
      break;
    }

    case STATE_SPEAKING: {
      pauseMic(); 
      delay(500);
      
      startTTS(aiResp.text_response);
      delay(200);
      
      unsigned long audioStartMs = millis();
      while (isAudioRunning()) {
        audioTick();
        oledTick();
        yield();
        if (millis() - audioStartMs > 15000) { break; }
      }
      
      delay(400);
      resumeMic(); 
      enterState(STATE_IDLE);
      break;
    }

    case STATE_ERROR:
      if (millis() - stateEnteredAt > 2500) {
        enterState(STATE_IDLE);
      }
      break;
  }
}