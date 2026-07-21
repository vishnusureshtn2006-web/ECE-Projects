#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <ESP32Servo.h>

// ================= WIFI =================

const char* ssid = "SmartDoor";
const char* password = "12345678";

WebServer server(80);

// ================= RFID =================

#define SS_PIN 21
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= FINGERPRINT =================

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ================= SERVO =================

Servo doorServo;

#define SERVO_PIN 25

const int LOCK_POS = 0;
const int UNLOCK_POS = 90;

// ================= USER DATA =================

String enrolledUID = "";
int enrolledFingerID = -1;

bool enrollRFIDMode = false;
bool enrollFingerMode = false;

// ================= DASHBOARD VARIABLES =================

String doorStatus = "LOCKED";
String systemStatus = "ONLINE";
String rfidStatus = "Waiting...";
String fingerStatus = "Waiting...";
String currentUser = "None";

String accessLogs[20];
int logIndex = 0;

void addLog(String msg)
{
  accessLogs[logIndex] = msg;
  logIndex++;

  if(logIndex >= 20)
    logIndex = 0;

  Serial.println(msg);
}

// ================= RFID =================

String readRFID()
{
  if (!rfid.PICC_IsNewCardPresent())
    return "";

  if (!rfid.PICC_ReadCardSerial())
    return "";

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
      uid += "0";

    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();

  rfid.PICC_HaltA();

  return uid;
}

// ================= FINGER =================

int getFingerprintID()
{
  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.fingerFastSearch();

  if (p != FINGERPRINT_OK)
    return -1;

  return finger.fingerID;
}

// ================= JSON STATUS =================

void handleStatus()
{
  String json = "{";

  json += "\"door\":\"" + doorStatus + "\",";
  json += "\"system\":\"" + systemStatus + "\",";
  json += "\"rfid\":\"" + rfidStatus + "\",";
  json += "\"finger\":\"" + fingerStatus + "\",";
  json += "\"user\":\"" + currentUser + "\"";

  json += "}";

  server.send(200, "application/json", json);
}


String html = R"rawliteral(
<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Smart Door Access System</title>

<style>

body{
background:#0f172a;
color:white;
font-family:Segoe UI;
padding:20px;
}

.card{
background:#1e293b;
padding:20px;
border-radius:15px;
margin-bottom:20px;
}

h1{
color:#38bdf8;
}

button{
padding:12px 20px;
border:none;
border-radius:10px;
background:#2563eb;
color:white;
cursor:pointer;
margin-right:10px;
}

button:hover{
opacity:0.9;
}

.status{
font-size:18px;
margin:8px 0;
}

</style>

</head>

<body>

<h1>SMART DOOR ACCESS SYSTEM</h1>

<div class="card">

<h2>Dashboard</h2>

<div class="status">
Door Status:
<span id="doorStatus">LOCKED</span>
</div>

<div class="status">
RFID Status:
<span id="rfidStatus">Waiting...</span>
</div>

<div class="status">
Fingerprint Status:
<span id="fingerStatus">Waiting...</span>
</div>

<div class="status">
Current User:
<span id="currentUser">None</span>
</div>

<div class="status">
System Status:
<span id="systemStatus">ONLINE</span>
</div>

</div>

<div class="card">

<h2>Enrollment</h2>

<button onclick="fetch('/enrollRFID')">
Enroll RFID
</button>

<button onclick="fetch('/enrollFinger')">
Enroll Fingerprint
</button>

</div>

<script>

setInterval(() => {

fetch('/status')
.then(response => response.json())
.then(data => {

document.getElementById('doorStatus').innerHTML = data.door;
document.getElementById('rfidStatus').innerHTML = data.rfid;
document.getElementById('fingerStatus').innerHTML = data.finger;
document.getElementById('currentUser').innerHTML = data.user;
document.getElementById('systemStatus').innerHTML = data.system;

});

},1000);

</script>

</body>
</html>
)rawliteral";
void handleRoot()
{
  server.send(200,"text/html",html);
}
void handleEnrollRFID()
{
  enrollRFIDMode = true;
  server.send(200,"text/plain","RFID Enrollment Started");
}

void handleEnrollFinger()
{
  enrollFingerMode = true;
  server.send(200,"text/plain","Fingerprint Enrollment Started");
}
void setup()
{
  Serial.begin(115200);

  SPI.begin(18,19,23,21);
  rfid.PCD_Init();

  mySerial.begin(57600,SERIAL_8N1,16,17);

  if(!finger.verifyPassword())
  {
    Serial.println("Fingerprint Sensor NOT Found");

    while(1);
  }

  doorServo.attach(SERVO_PIN);
  doorServo.write(LOCK_POS);

  WiFi.softAP(ssid,password);

  Serial.println(WiFi.softAPIP());
  server.on("/enrollRFID",handleEnrollRFID);
server.on("/enrollFinger",handleEnrollFinger);
  server.on("/",handleRoot);
  server.on("/status",handleStatus);

  server.begin();

  addLog("System Started");
}
void loop()
{
  server.handleClient();

  String uid = readRFID();
  if(enrollRFIDMode)
{
  String uid = readRFID();

  if(uid != "")
  {
    enrolledUID = uid;
    enrollRFIDMode = false;

    rfidStatus = "RFID Saved";

    addLog("RFID Enrolled: " + uid);
  }
}

if(enrollFingerMode)
{
  fingerStatus = "Enroll Finger";

  int id = getFingerprintID();

  if(id > 0)
  {
    enrolledFingerID = id;
    enrollFingerMode = false;

    fingerStatus = "Fingerprint Saved";

    addLog("Finger Saved ID: " + String(id));
  }
}

if(uid == "")
{
    delay(10);
    return;
}
  rfidStatus = "RFID Detected";

  if(uid != enrolledUID)
  {
    rfidStatus = "Unknown RFID";
    currentUser = "Unknown";

    addLog("Access Denied RFID");

    delay(2000);

    rfidStatus = "Waiting...";
    return;
  }

  currentUser = "Vishnu";
  rfidStatus = "RFID Verified";

  fingerStatus = "Place Finger";

  unsigned long startTime = millis();

  bool fingerMatched = false;

  while(millis() - startTime < 10000)
  {
    server.handleClient();

    int fingerID = getFingerprintID();

    if(fingerID == enrolledFingerID)
    {
      fingerMatched = true;
      break;
    }

    delay(50);
  }

  if(!fingerMatched)
  {
    fingerStatus = "Fingerprint Failed";

    addLog("Fingerprint Failed");

    delay(2000);

    fingerStatus = "Waiting...";
    return;
  }

  fingerStatus = "Verified";

  doorStatus = "UNLOCKED";

  addLog("Access Granted");

  doorServo.write(UNLOCK_POS);

  delay(5000);

  doorServo.write(LOCK_POS);

  doorStatus = "LOCKED";
  currentUser = "None";
  rfidStatus = "Waiting...";
  fingerStatus = "Waiting...";
}