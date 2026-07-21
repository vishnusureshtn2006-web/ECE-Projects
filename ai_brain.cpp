// ============================================================
//  ai_brain.cpp  —  Calls Gemini, parses structured JSON
// ============================================================
#include "ai_brain.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// System prompt updated to match "Adrian"
static const char* SYSTEM_PROMPT =
  "You are Adrian, a friendly robotic voice assistant built on an ESP32-S3. "
  "Answer clearly and concisely in 1 or 2 sentences.";

AiResponse askAI(String userText) {
  AiResponse resp;

  if (WiFi.status() != WL_CONNECTED) {
    resp.text_response = "WiFi is not connected.";
    resp.emotion = "confused";
    return resp;
  }

  HTTPClient http;
  
  // Endpoint using gemini-2.5-flash (Match with config/README)
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + String(GEMINI_API_KEY);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(20000);

  // Build JSON Payload using modern ArduinoJson / v6 structure
  DynamicJsonDocument req(2048);

  // System Instruction Payload
  JsonObject sysInstruction = req.createNestedObject("system_instruction");
  JsonObject sysParts = sysInstruction.createNestedArray("parts").createNestedObject();
  sysParts["text"] = SYSTEM_PROMPT;

  // User Content Payload
  JsonArray contents = req.createNestedArray("contents");
  JsonObject turn = contents.createNestedObject();
  turn["role"] = "user";
  JsonArray parts = turn.createNestedArray("parts");
  parts.createNestedObject()["text"] = userText;

  String body;
  serializeJson(req, body);

  Serial.println("[AI] Sending: " + userText);
  int code = http.POST(body);

  // Retry once on 429 rate limit
  if (code == 429) {
    Serial.println("[AI] Rate limited, waiting 5s...");
    http.end();
    delay(5000);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    code = http.POST(body);
  }

  if (code == 200) {
    String rawResp = http.getString();
    Serial.println("[AI] Raw: " + rawResp.substring(0, 150) + "...");

    DynamicJsonDocument outer(8192);
    DeserializationError err = deserializeJson(outer, rawResp);

    if (!err) {
      // Safe extraction check to avoid crashes on missing keys
      const char* extractedText = outer["candidates"][0]["content"]["parts"][0]["text"];
      
      if (extractedText) {
        String textStr = String(extractedText);
        textStr.trim();
        
        resp.text_response = textStr;
        resp.emotion = "talking";
        resp.speaker_tone = "neutral";
        resp.animation = "talking";
      } else {
        Serial.println("[AI] Error: Text field missing in response JSON.");
        resp.text_response = "I couldn't generate a text response.";
        resp.emotion = "confused";
      }
    } else {
      Serial.println("[AI] JSON Parse Failed: " + String(err.c_str()));
      resp.text_response = "Failed to parse AI response.";
      resp.emotion = "confused";
    }
  } else {
    Serial.println("[AI] HTTP error: " + String(code));
    resp.text_response = "I had trouble reaching the AI. Error " + String(code);
    resp.emotion = "confused";
  }

  http.end();
  return resp;
}