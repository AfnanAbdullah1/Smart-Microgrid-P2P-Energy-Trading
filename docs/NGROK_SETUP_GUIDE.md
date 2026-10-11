# Ngrok Setup Guide for Smart Microgrid

By default, the ESP32 controllers must be on the exact same WiFi network as the PC running the backend server. 

If you want to run the ESP32s on a different network (like a mobile hotspot) or share your live dashboard with someone remotely, you can use **Ngrok** to create a secure, public HTTPS URL for your local server.

## 1. Download and Install Ngrok

1. Go to the [Ngrok Download Page](https://ngrok.com/download).
2. Download the version for your operating system (e.g., Windows 64-bit).
3. Extract the downloaded ZIP file. You will get an executable file (`ngrok.exe` on Windows).

## 2. Authenticate Your Ngrok Account

1. Sign up for a free account at [ngrok.com](https://ngrok.com).
2. Go to your [Ngrok Dashboard](https://dashboard.ngrok.com/get-started/your-authtoken).
3. Copy your specific Authtoken.
4. Open a Command Prompt or PowerShell in the folder where you extracted `ngrok.exe`.
5. Run the following command (replace `<YOUR_AUTHTOKEN>` with your actual token):

```powershell
ngrok config add-authtoken <YOUR_AUTHTOKEN>
```

You only need to do this step once.

## 3. Run Ngrok

Your Smart Microgrid backend server runs on Port 3000. To expose this port to the internet, run the following command in your Command Prompt/PowerShell where `ngrok.exe` is located:

```powershell
ngrok http 3000
```

You will see an interface appear in your terminal looking something like this:

```
ngrok                                                           (Ctrl+C to quit)

Session Status                online
Account                       Your Name (Plan: Free)
Version                       3.5.0
Region                        United States (us)
Latency                       45ms
Web Interface                 http://127.0.0.1:4040
Forwarding                    https://1234-56-78-910.ngrok-free.app -> http://localhost:3000
```

## 4. Update the ESP32 Firmware

1. **Copy the Forwarding URL:** Select and copy the `https://...` URL from the `Forwarding` line in the Ngrok terminal (e.g., `https://1234-56-78-910.ngrok-free.app`). Do not include any trailing slash `/`.
2. **Open the `secrets.h` file** for both `house_a_controller` and `house_b_controller`.
3. Locate the `SERVER CONFIGURATION` section.
4. Uncomment the `NGROK_URL` line and paste your copied URL:

```cpp
// Option 2: Ngrok Public URL (Over internet)
// Uncomment this line and paste your Ngrok URL (WITHOUT a trailing slash)
#define NGROK_URL "https://1234-56-78-910.ngrok-free.app"
```

5. Re-upload the firmware to your House A and House B ESP32 boards using the Arduino IDE.

## 5. Important Notes

- **Keep the terminal open:** You must leave the terminal window running `ngrok http 3000` open for the connection to work. If you close it, the URL will stop working.
- **Dynamic URLs:** On the free tier of Ngrok, the URL changes every time you restart the Ngrok command. If you restart Ngrok, you will need to copy the *new* URL, update both `secrets.h` files, and re-upload the code to both ESP32s.
- **Backend must be running:** Ngrok only creates the tunnel. You still need to make sure your backend server (`npm start`) is running in a separate terminal window.
