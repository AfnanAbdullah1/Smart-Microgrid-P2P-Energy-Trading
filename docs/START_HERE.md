# 🚀 How to Start the Project

## Step 1: Start Ganache (Blockchain)

1. Open **Ganache** app
2. Click your saved workspace (or create new → Port **7545**, Chain ID **1337**)
3. Leave it running


---

## Step 3: Start Backend Server     

open terminal

```powershell
cd backend
npm start
```

You should see:
```
✓ Connected to Ganache
✓ Server running on port 3000
```

---

## Step 4: Start Ngrok Tunnel

Open a **new terminal** and run:

```powershell
ngrok http 3000
```

Your static URL: `https://proreduction-unevangelical-suzan.ngrok-free.dev`

> This lets your ESP32s reach the server from **any WiFi network** — no need to be on the same network as your PC. Since you have a **static domain**, the URL never changes.

---

## Step 5: Configure ESP32 Firmware (First time only)

Edit `firmware/house_a_controller/secrets.h` and `firmware/house_b_controller/secrets.h`:

```cpp
// WiFi (your phone hotspot or any WiFi the ESP32 connects to)
#define WIFI_SSID "YourWiFi"
#define WIFI_PASSWORD "YourPassword"

// Your static Ngrok URL (already configured)
#define NGROK_URL "https://proreduction-unevangelical-suzan.ngrok-free.dev"
```

> Since the ngrok URL is **static**, you only need to set this once.

---

## Step 6: Upload Firmware

In **Arduino IDE**:

1. Install **ESP32 board** (Boards Manager → search "esp32")
2. Install libraries: **PZEM-004T-v30**, **ArduinoJson**, **LiquidCrystal_I2C**
3. Open `firmware/house_a_controller/house_a_controller.ino`
4. Select Board: **ESP32 Dev Module**, select COM port
5. Upload ⬆️
6. Repeat for `house_b_controller.ino` on the second ESP32

---

## Step 7: Test

1. Remove all loads from House A (0W consumption)
2. Relay should turn **ON** (consumption < 1W threshold)
3. Check **dashboard**: open your ngrok URL in browser
4. Add a 40W bulb → relay turns **OFF**

---

## Wiring Quick Reference

| Connection | Pin |
|-----------|-----|
| House A PZEM TX → ESP32 | GPIO 18 |
| House A PZEM RX → ESP32 | GPIO 19 |
| Relay control | GPIO 27 |
| House B PZEM TX → ESP32 | GPIO 16 |
| House B PZEM RX → ESP32 | GPIO 17 |
| LCD SDA → ESP32 | GPIO 21 |
| LCD SCL → ESP32 | GPIO 22 |
| LCD VCC | 5V |
| LCD GND | GND |

---

## Daily Startup Checklist

```
1. Open Ganache
2. cd backend → npm start
3. ngrok http 3000 (new terminal)
4. Power on ESP32s (auto-connect via static ngrok URL)
5. Open https://proreduction-unevangelical-suzan.ngrok-free.dev for dashboard
```

That's it! 🎉
