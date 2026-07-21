#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- Network & API Config ---
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_GEMINI_API_KEY"; 

// Gemini endpoint
const char* gemini_endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=";

// Google's Root CA Certificate (Enables secure HTTPS connection)
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFVzCCAz6gAwIBAgIQB3v6v3f+LAnRAnWvK3F8DzANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBYGA1UEChMPR29vZ2xlIFRydXN0IFNlcnZpY2VzMSUw\n" \
"IwYDVQQDExxHb29nbGUgVHJ1c3QgU2VydmljZXMgR0NTUiAxMB4XDTI0MDExNTAw\n" \
"MDAwMFoXDTI5MDExNTAwMDAwMFowRzELMAkGA1UEBhMCVVMxFTATBgNVBAoMTEdv\n" \
"b2dsZSBMTEMxHzAdBgNVBAMMFnd3dy5nb29nbGVhcGlzLmNvbSNDQTASBgNVHRMB\n" \
"Af8ECDAGAQH/AgEAMB0GA1UdDgQWBBQyVzJbW9+8V... (or leave insecure for testing)\n" \
"-----END CERTIFICATE-----\n";

void setup() {
  Serial.begin(115200);
  delay(2000); // Give the S3 time to initialize serial

  Serial.println("\nInitializing ESP32-S3...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi successfully!");

  // Example translation sentence
  String englishPhrase = "Artificial Intelligence is evolving rapidly every single day.";
  
  Serial.println("\n--- English to Hindi ---");
  Serial.print("Input:  "); Serial.println(englishPhrase);
  Serial.println("Translating via Gemini...");

  String result = translateToHindi(englishPhrase);
  
  Serial.print("Output: "); Serial.println(result);
}

void loop() {
  // Static execution for setup testing
}

String translateToHindi(String text) {
  if (WiFi.status() != WL_CONNECTED) return "Wi-Fi Disconnected";

  WiFiClientSecure client;
  // For production, use client.setCACert(root_ca); 
  // For quick testing, we can tell the S3 to skip strict certificate validation:
  client.setInsecure(); 

  HTTPClient http;
  String url = String(gemini_endpoint) + apiKey;
  
  if (!http.begin(client, url)) {
    return "Connection to endpoint failed";
  }
  
  http.addHeader("Content-Type", "application/json");

  // Format payload for Gemini 2.5
  JsonDocument doc;
  
  // Set explicit rules via system instruction
  JsonObject sysInstruction = doc["system_instruction"].to<JsonObject>();
  sysInstruction["parts"][0]["text"] = "You are a precise English-to-Hindi translator. Translate the text accurately into proper Hindi script. Output ONLY the translated text.";

  // Set user text
  doc["contents"][0]["parts"][0]["text"] = text;
  
  // Setting a lower temperature prevents creative hallucinations during translation
  doc["generationConfig"]["temperature"] = 0.2; 

  String requestBody;
  serializeJson(doc, requestBody);

  int httpResponseCode = http.POST(requestBody);
  String translatedString = "";

  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();
    JsonDocument responseDoc;
    deserializeJson(responseDoc, response);
    
    const char* extractedText = responseDoc["candidates"][0]["content"]["parts"][0]["text"];
    if (extractedText) {
      translatedString = String(extractedText);
      translatedString.trim();
    } else {
      translatedString = "Parsing Error: Content path not found.";
    }
  } else {
    translatedString = "HTTP Error: " + String(httpResponseCode);
  }

  http.end();
  return translatedString;
}