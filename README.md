# RFID Lock & Key

An ESP32-based RFID door lock that grants or denies access based on a whitelisted card UID, controls a servo lock, triggers LED/buzzer feedback, and logs every scan to a Google Sheet.

## Hardware

| Component | Pin |
|-----------|-----|
| MFRC522 RFID reader (SS) | GPIO 5 |
| MFRC522 RFID reader (RST) | GPIO 21 |
| Servo motor | GPIO 14 |
| Green LED | GPIO 26 |
| Red LED | GPIO 27 |
| Buzzer | GPIO 25 |

## How it works

1. On boot the ESP32 connects to Wi-Fi and initializes the RFID reader with the servo in the locked position.
2. When a card is tapped, the UID is compared against the hardcoded authorized UID.
3. **Authorized** — green LED on, single high beep, servo rotates to 90° for 3 seconds then re-locks.
4. **Denied** — red LED on, two short low beeps.
5. Either way, the UID and result (`AUTHORIZED` / `DENIED`) are sent to a Google Apps Script web app that logs the entry to a Google Sheet.

## Dependencies

Install these libraries via the Arduino Library Manager:

- `MFRC522` by GithubCommunity
- `ESP32Servo` by Kevin Harrington
- `WiFi` / `HTTPClient` (bundled with the ESP32 Arduino core)

## Configuration

Before flashing, update the following constants in `rfid_Test.ino`:

```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

const char* SCRIPT_URL = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec";

byte authorizedUID[] = { 0xXX, 0xXX, 0xXX, 0xXX };
```

To find your card's UID, flash the sketch and open the Serial Monitor (115200 baud) — the UID is printed on every scan.

## Google Sheets logging

The sketch calls the Apps Script URL with two query parameters:

```
?uid=<UID>&result=<AUTHORIZED|DENIED>
```

Deploy your Apps Script as a web app with "Anyone" access and paste the URL into `SCRIPT_URL`.
