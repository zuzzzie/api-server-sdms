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
//   Airtel India : "airtelgprs.com"
//   Jio          : "jionet"
//   BSNL         : "bsnlnet"
//   Vi/Idea      : "internet"
const char apn[]  = "airtelgprs.com";
const char user[] = "";
const char pass[] = "";

// ─── Railway API Settings ──────────────────
// Plain HTTP on port 80 — Railway edge accepts HTTP and forwards it
// internally to the Node.js container.  No HTTPS needed on the device.
// POST /data requires NO API key — the server uses rate-limiting instead.
const char* SERVER    = "api-server-sdms-production.up.railway.app";
const int   PORT      = 80;
const char* ENDPOINT  = "/data";
const char* DEVICE_ID = "bin-01";
const float FULL_CM   = 10.0;

// ─── Fixed GPS Coordinates ─────────────────
// Replace with your bin's actual GPS position.
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
  Serial.println("  Smart Bin → Railway + Supabase");
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
    Serial.println("Network failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("Network OK!");

  // ─── Signal Quality ────────────────────
  int signal = modem.getSignalQuality();
  Serial.print("Signal Quality: ");
  Serial.println(signal);

  // ─── GPRS ──────────────────────────────
  Serial.println("\n--- Connecting GPRS ---");
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println("GPRS Failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("GPRS Connected!");
  Serial.print("IP Address: ");
  Serial.println(modem.localIP());

  // ─── Server Reachability Check ─────────
  // Uses a plain TCP connect — confirms the server IP is routable.
  Serial.println("\n--- Checking Railway Server ---");
  TinyGsmClient testClient(modem);
  if (testClient.connect(SERVER, PORT)) {
    Serial.println("Railway server reachable!");
    testClient.stop();
  } else {
    Serial.println("Server not reachable yet — continuing anyway");
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
void sendData(float dist, bool full) {

  // ─── GPRS Reconnect if Dropped ─────────
  if (!modem.isGprsConnected()) {
    Serial.println("GPRS dropped! Reconnecting...");
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println("GPRS reconnect failed — skipping this reading");
      return;
    }
    Serial.println("GPRS reconnected!");
    delay(2000);
  }

  // ─── Build JSON Payload ────────────────
  // Fields match exactly what POST /data expects on the Railway server.
  // The server validates each field and writes to Supabase bin_locations.
  // fill_level (0-100%) is computed server-side from distance.
  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"lat\":"         + String(FIXED_LAT, 6) + ",";
  payload += "\"lng\":"         + String(FIXED_LNG, 6) + ",";
  payload += "\"distance\":"    + String(dist, 2)      + ",";
  payload += "\"bin_full\":"    + (full ? "true" : "false") + ",";
  payload += "\"device_ts\":"   + String(millis());
  payload += "}";

  Serial.println("Payload  : " + payload);
  Serial.println("Sending to Railway...");

  // ─── Fresh Client Every Send ───────────
  // Re-creating the client each cycle avoids stale-socket status -3 errors.
  // Plain TinyGsmClient = HTTP (no TLS). Railway edge accepts plain HTTP
  // on port 80 and routes it to the Node.js container internally.
  TinyGsmClient freshClient(modem);
  HttpClient    http(freshClient, SERVER, PORT);
  http.setTimeout(15000);  // 15 s — GSM latency is high

  // ArduinoHttpClient automatically adds the correct Host: header,
  // which is required for Railway's SNI-based routing on shared IPs.
  int err = http.post(ENDPOINT, "application/json", payload);

  if (err != 0) {
    Serial.println("HTTP error: " + String(err) + " — will retry next cycle");
    http.stop();
    return;
  }

  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  // ─── Response Handling ────────────────
  if (status == 200) {
    Serial.println("Saved to Supabase via Railway!");
  } else if (status == 301 || status == 302) {
    // Railway redirected HTTP → HTTPS. This means it did not accept plain
    // HTTP on this path. Contact support or use TinyGsmClientSecure instead.
    Serial.println("Redirect (3xx) — server may require HTTPS. Status: " + String(status));
  } else if (status == 400) {
    Serial.println("Bad request (400) — check payload fields match server schema");
    Serial.println("   Response: " + resp);
  } else if (status == 429) {
    Serial.println("Rate limited (429) — sending too fast, increase delay");
  } else {
    Serial.println("Unexpected status: " + String(status));
    Serial.println("   Response: " + resp);
  }
}

// ───────────────────────────────────────────
void loop() {
  if (!modemReady) {
    Serial.println("Modem not ready! Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("─────────────────────────────────");

  // ─── Read Ultrasonic Sensor ────────────
  float dist = getDistance();

  if (dist < 0) {
    Serial.println("Sensor error — skipping this reading");
    delay(2000);
    return;
  }

  bool full = (dist < FULL_CM);

  // ─── Print Readings ────────────────────
  Serial.print("Distance : ");
  Serial.print(dist);
  Serial.println(" cm");

  Serial.print("Bin Full : ");
  Serial.println(full ? "YES" : "NO");

  Serial.print("Location : ");
  Serial.print(FIXED_LAT, 6);
  Serial.print(", ");
  Serial.println(FIXED_LNG, 6);

  // ─── Send to Railway → Supabase ────────
  sendData(dist, full);

  Serial.println("reading in 60 seconds...");
  Serial.println("─────────────────────────────────\n");
  delay(60000);
}