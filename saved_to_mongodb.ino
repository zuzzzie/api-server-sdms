#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <HardwareSerial.h>

// ─── SIM800L Pins ──────────────────────────
#define MODEM_TX        27
#define MODEM_RX        26
#define MODEM_PWRKEY     4
#define MODEM_POWER_ON  23
#define MODEM_RST        5

// ─── Ultrasonic Pins ───────────────────────
#define TRIG_PIN        12
#define ECHO_PIN        13

// ─── APN Settings ──────────────────────────
const char apn[]  = "airtelgprs.com";
const char user[] = "";
const char pass[] = "";

// ─── AWS EC2 Settings ──────────────────────
const char* SERVER    = "13.202.7.165";
const int   PORT      = 80;
const char* ENDPOINT  = "/data";
const char* DEVICE_ID = "bin-01";
const float FULL_CM   = 10.0;

// ─── Fixed GPS Coordinates ─────────────────
const float FIXED_LAT = 23.892550;
const float FIXED_LNG = 89.595650;

// ─── Objects ───────────────────────────────
HardwareSerial SerialAT(1);
TinyGsm        modem(SerialAT);

bool modemReady = false;

// ───────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // ─── Ultrasonic Setup ──────────────────
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("================================");
  Serial.println("   Smart Bin → AWS EC2 Cloud    ");
  Serial.println("================================");
  Serial.println("Initializing GSM...");

  // ─── Modem Pin Setup ───────────────────
  pinMode(MODEM_PWRKEY,   OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(MODEM_RST,      OUTPUT);

  digitalWrite(MODEM_POWER_ON, HIGH);
  digitalWrite(MODEM_RST,      HIGH);

  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // ─── Power ON Sequence ─────────────────
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(2000);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000);

  // ─── Modem Info ────────────────────────
  Serial.println("\n--- Modem Info ---");
  Serial.println(modem.getModemInfo());

  // ─── Network ───────────────────────────
  Serial.println("\n--- Waiting for Network ---");
  if (!modem.waitForNetwork()) {
    Serial.println("❌ Network failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("✅ Network OK!");

  // ─── Signal Quality ────────────────────
  int signal = modem.getSignalQuality();
  Serial.print("✅ Signal Quality: ");
  Serial.println(signal);

  // ─── GPRS ──────────────────────────────
  Serial.println("\n--- Connecting GPRS ---");
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println("❌ GPRS Failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("✅ GPRS Connected!");
  Serial.print("✅ IP Address: ");
  Serial.println(modem.localIP());

  // ─── EC2 Reachability Check ────────────
  Serial.println("\n--- Checking EC2 Server ---");
  TinyGsmClient testClient(modem);
  if (testClient.connect(SERVER, PORT)) {
    Serial.println("✅ EC2 server reachable!");
    testClient.stop();
  } else {
    Serial.println("⚠️  EC2 not reachable yet — continuing anyway");
  }

  Serial.println("\n================================");
  Serial.println("  Setup Done! Sending Data...   ");
  Serial.println("================================\n");

  modemReady = true;
}

// ───────────────────────────────────────────
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2.0;
}

// ───────────────────────────────────────────
void sendToAWS(float dist, bool full) {

  // ─── GPRS Reconnect if Dropped ─────────
  if (!modem.isGprsConnected()) {
    Serial.println("⚠️  GPRS dropped! Reconnecting...");
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println("❌ GPRS reconnect failed — skipping this reading");
      return;
    }
    Serial.println("✅ GPRS reconnected!");
    delay(2000);
  }

  // ─── Build JSON Payload ────────────────
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"lat\":"         + String(FIXED_LAT, 6) + ",";
  payload += "\"lng\":"         + String(FIXED_LNG, 6) + ",";
  payload += "\"distance\":"    + String(dist, 2)      + ",";
  payload += "\"bin_full\":"    + String(full ? "true" : "false") + ",";
  payload += "\"device_ts\":"   + String(millis());
  payload += "}";

  Serial.println("📦 Payload  : " + payload);
  Serial.println("📡 Sending to AWS EC2...");

  // ─── Fresh Client Every Send ───────────
  // (Fixes status -3 / stale connection bug)
  TinyGsmClient freshClient(modem);
  HttpClient    http(freshClient, SERVER, PORT);
  http.setTimeout(10000);  // 10 second timeout

  // ─── Single POST — No Retry Loop ───────
  int err = http.post(ENDPOINT, "application/json", payload);

  if (err != 0) {
    Serial.println("❌ Connection error: " + String(err) + " — will retry next cycle");
    http.stop();
    return;
  }

  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  // ─── Result ────────────────────────────
  if (status == 200) {
    Serial.println("✅ Saved to MongoDB successfully!");
  } else {
    Serial.println("❌ Server returned status: " + String(status));
    Serial.println("   Response : " + resp);
  }
}

// ───────────────────────────────────────────
void loop() {
  if (!modemReady) {
    Serial.println("❌ Modem not ready! Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("─────────────────────────────────");

  // ─── Read Ultrasonic Sensor ────────────
  float dist = getDistance();

  if (dist < 0) {
    Serial.println("⚠️  Sensor error — skipping this reading");
    delay(2000);
    return;
  }

  bool full = (dist < FULL_CM);

  // ─── Print Readings ────────────────────
  Serial.print("📏 Distance : ");
  Serial.print(dist);
  Serial.println(" cm");

  Serial.print("🗑️  Bin Full : ");
  Serial.println(full ? "YES 🔴" : "NO  🟢");

  Serial.print("📍 Location : ");
  Serial.print(FIXED_LAT, 6);
  Serial.print(", ");
  Serial.println(FIXED_LNG, 6);

  // ─── Send Once to AWS ──────────────────
  sendToAWS(dist, full);

  Serial.println("⏳ Next reading in 5 seconds...");
  Serial.println("─────────────────────────────────\n");
  delay(5000);
}