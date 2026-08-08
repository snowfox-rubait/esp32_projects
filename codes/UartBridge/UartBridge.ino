#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

// Preferences for non-volatile storage
Preferences preferences;

// Configuration variables
String wifi_ssid = "";
String wifi_pass = "";
unsigned long baud_rate = 115200;
bool pins_swapped = false;

// Default pins (Hardware UART2)
int pin_rx = 16;
int pin_tx = 17;

// Servers
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// UART buffers
#define BUFFER_SIZE 1024
char rx_buf[BUFFER_SIZE];

// HTML UI code stored in flash
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 UART Control Panel</title>
    <link href="https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;500;700&family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-color: #0b0f19;
            --card-bg: rgba(22, 28, 45, 0.6);
            --primary: #8b5cf6;
            --primary-hover: #7c3aed;
            --accent-green: #10b981;
            --accent-red: #f43f5e;
            --text-color: #f8fafc;
            --text-muted: #64748b;
            --border-color: rgba(255, 255, 255, 0.05);
            --input-bg: #1e293b;
            --terminal-bg: #020617;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: 'Inter', sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
            background-image: radial-gradient(circle at 0% 0%, rgba(139, 92, 246, 0.08) 0%, transparent 50%),
                              radial-gradient(circle at 100% 100%, rgba(16, 185, 129, 0.05) 0%, transparent 50%);
        }

        .container {
            width: 100%;
            max-width: 1100px;
            background: var(--card-bg);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 24px;
            border: 1px solid var(--border-color);
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            overflow: hidden;
            display: grid;
            grid-template-rows: auto 1fr auto;
        }

        /* Header */
        .header {
            padding: 24px 32px;
            border-bottom: 1px solid var(--border-color);
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: rgba(0, 0, 0, 0.15);
        }

        .title-area h1 {
            font-size: 1.6rem;
            font-weight: 700;
            letter-spacing: -0.02em;
            background: linear-gradient(to right, #a78bfa, #34d399);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        .title-area p {
            font-size: 0.85rem;
            color: var(--text-muted);
            margin-top: 4px;
            font-weight: 500;
        }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 10px;
            font-size: 0.85rem;
            font-weight: 600;
            padding: 8px 16px;
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid var(--border-color);
            border-radius: 100px;
        }

        .status-dot {
            width: 10px;
            height: 10px;
            background-color: var(--accent-red);
            border-radius: 50%;
            box-shadow: 0 0 10px var(--accent-red);
            transition: all 0.3s ease;
        }

        .status-dot.connected {
            background-color: var(--accent-green);
            box-shadow: 0 0 12px var(--accent-green);
        }

        /* Main Workspace */
        .workspace {
            display: grid;
            grid-template-columns: 350px 1fr;
            height: 600px;
        }

        @media (max-width: 900px) {
            .workspace {
                grid-template-columns: 1fr;
                height: auto;
            }
        }

        /* Sidebar Controls */
        .sidebar {
            padding: 32px;
            border-right: 1px solid var(--border-color);
            display: flex;
            flex-direction: column;
            gap: 28px;
            overflow-y: auto;
            background: rgba(0, 0, 0, 0.08);
        }

        .control-group {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }

        .control-group label {
            font-size: 0.75rem;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            color: var(--text-muted);
        }

        .input-field {
            width: 100%;
            background: var(--input-bg);
            border: 1px solid var(--border-color);
            color: var(--text-color);
            padding: 12px 14px;
            border-radius: 12px;
            outline: none;
            font-size: 0.9rem;
            transition: all 0.2s;
        }

        .input-field:focus {
            border-color: var(--primary);
            box-shadow: 0 0 0 2px rgba(139, 92, 246, 0.15);
        }

        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 12px;
            font-weight: 600;
            font-size: 0.9rem;
            cursor: pointer;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            transition: all 0.2s ease;
        }

        .btn:hover {
            background: var(--primary-hover);
            transform: translateY(-1px);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn-secondary {
            background: rgba(255, 255, 255, 0.03);
            color: var(--text-color);
            border: 1px solid var(--border-color);
        }

        .btn-secondary:hover {
            background: rgba(255, 255, 255, 0.08);
        }

        /* Pinout Monitor */
        .pinout-box {
            background: rgba(15, 23, 42, 0.4);
            border: 1px dashed var(--border-color);
            border-radius: 12px;
            padding: 14px;
            display: flex;
            justify-content: space-around;
            align-items: center;
            font-family: 'Fira Code', monospace;
            font-size: 0.85rem;
        }

        .pin-val {
            font-weight: 700;
            color: var(--primary);
        }

        /* Console */
        .console-area {
            padding: 32px;
            display: flex;
            flex-direction: column;
            gap: 20px;
            height: 100%;
        }

        .terminal-container {
            flex-grow: 1;
            position: relative;
            min-height: 0;
        }

        .terminal {
            width: 100%;
            height: 100%;
            background-color: var(--terminal-bg);
            border: 1px solid var(--border-color);
            border-radius: 16px;
            padding: 20px;
            overflow-y: auto;
            font-family: 'Fira Code', monospace;
            font-size: 0.9rem;
            line-height: 1.6;
            color: var(--accent-green);
            white-space: pre-wrap;
            box-shadow: inset 0 4px 20px rgba(0, 0, 0, 0.6);
        }

        /* Command bar */
        .cmd-bar {
            display: flex;
            gap: 12px;
        }

        /* Footer */
        .footer {
            padding: 16px 32px;
            border-top: 1px solid var(--border-color);
            display: flex;
            justify-content: space-between;
            font-size: 0.75rem;
            color: var(--text-muted);
            font-weight: 500;
            background: rgba(0, 0, 0, 0.15);
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <div class="title-area">
                <h1>ESP32 UART Control Panel</h1>
                <p>Advanced Wireless Pass-Through Console</p>
            </div>
            <div class="status-badge">
                <div class="status-dot" id="status-dot"></div>
                <span id="status-text">Connecting...</span>
            </div>
        </div>

        <!-- Workspace -->
        <div class="workspace">
            <!-- Sidebar Controls -->
            <div class="sidebar">
                <!-- Wi-Fi settings -->
                <div class="control-group">
                    <label>Station Wi-Fi Configuration</label>
                    <input type="text" id="wifi-ssid" class="input-field" placeholder="Wi-Fi SSID">
                    <input type="password" id="wifi-pass" class="input-field" placeholder="Wi-Fi Password">
                    <button class="btn" onclick="saveWifi()">Save & Connect</button>
                </div>

                <!-- Baud rates -->
                <div class="control-group">
                    <label>Baud Rate</label>
                    <select id="baud-preset" class="input-field" onchange="presetBaudChange()">
                        <option value="9600">9600 bps</option>
                        <option value="19200">19200 bps</option>
                        <option value="38400">38400 bps</option>
                        <option value="57600">57600 bps</option>
                        <option value="115200" selected>115200 bps</option>
                        <option value="230400">230400 bps</option>
                        <option value="460800">460800 bps</option>
                        <option value="921600">921600 bps</option>
                        <option value="custom">-- Custom Baud --</option>
                    </select>
                    <div id="custom-baud-wrapper" style="display: none; margin-top: 8px;">
                        <input type="number" id="baud-custom" class="input-field" placeholder="Enter Custom Baud">
                        <button class="btn btn-secondary" style="width: 100%; margin-top: 8px;" onclick="applyCustomBaud()">Apply Custom</button>
                    </div>
                </div>

                <!-- Pin mapping -->
                <div class="control-group">
                    <label>UART Pin Mapping</label>
                    <div class="pinout-box">
                        <div>TX: <span id="tx-pin" class="pin-val">GPIO 17</span></div>
                        <div>RX: <span id="rx-pin" class="pin-val">GPIO 16</span></div>
                    </div>
                    <button class="btn btn-secondary" onclick="swapPins()">Swap TX / RX</button>
                </div>
            </div>

            <!-- Terminal Console -->
            <div class="console-area">
                <div class="terminal-container">
                    <div class="terminal" id="terminal-log">Connecting to ESP32 UART Server...</div>
                </div>
                <div class="cmd-bar">
                    <input type="text" id="cmd-input" class="input-field" style="font-family: 'Fira Code', monospace;" placeholder="Type payload to transmit..." onkeydown="handleInputKey(event)">
                    <button class="btn" onclick="sendTxPayload()">Transmit</button>
                    <button class="btn btn-secondary" onclick="clearLogs()">Clear</button>
                </div>
            </div>
        </div>

        <!-- Footer -->
        <div class="footer">
            <span id="footer-connection">Mode: Standalone AP</span>
            <span>Created by Antigravity Agent</span>
        </div>
    </div>

    <script>
        let ws;
        const terminalLog = document.getElementById('terminal-log');
        const cmdInput = document.getElementById('cmd-input');
        const statusDot = document.getElementById('status-dot');
        const statusText = document.getElementById('status-text');
        const txPinSpan = document.getElementById('tx-pin');
        const rxPinSpan = document.getElementById('rx-pin');
        const wifiSsidInput = document.getElementById('wifi-ssid');
        const wifiPassInput = document.getElementById('wifi-pass');
        const baudPresetSelect = document.getElementById('baud-preset');
        const customBaudWrapper = document.getElementById('custom-baud-wrapper');
        const baudCustomInput = document.getElementById('baud-custom');
        const footerConnection = document.getElementById('footer-connection');

        function appendLog(text, color = "") {
            const span = document.createElement('span');
            if (color) span.style.color = color;
            span.textContent = text;
            terminalLog.appendChild(span);
            terminalLog.scrollTop = terminalLog.scrollHeight;
        }

        function clearLogs() {
            terminalLog.innerHTML = "";
        }

        function connectWs() {
            const host = window.location.hostname || "192.168.4.1";
            ws = new WebSocket(`ws://${host}:81/`);

            ws.onopen = () => {
                statusDot.className = "status-dot connected";
                statusText.innerText = "Connected";
                clearLogs();
                appendLog("--- Dynamic Console Session Established ---\n", "#10b981");
            };

            ws.onmessage = (event) => {
                const data = event.data;
                if (data.startsWith("STATE:")) {
                    const parts = data.substring(6).split(",");
                    txPinSpan.innerText = "GPIO " + parts[0];
                    rxPinSpan.innerText = "GPIO " + parts[1];
                    
                    const baud = parts[2];
                    const wifiMode = parts[3];
                    const wifiIp = parts[4];

                    let hasPreset = false;
                    for (let opt of baudPresetSelect.options) {
                        if (opt.value === baud) {
                            baudPresetSelect.value = baud;
                            hasPreset = true;
                            break;
                        }
                    }
                    if (!hasPreset) {
                        baudPresetSelect.value = "custom";
                        customBaudWrapper.style.display = "block";
                        baudCustomInput.value = baud;
                    } else {
                        customBaudWrapper.style.display = "none";
                    }

                    if (wifiMode === "STA") {
                        footerConnection.innerText = `Mode: STA (IP: ${wifiIp})`;
                    } else {
                        footerConnection.innerText = `Mode: Standalone Access Point (192.168.4.1)`;
                    }
                } else {
                    appendLog(data);
                }
            };

            ws.onclose = () => {
                statusDot.className = "status-dot";
                statusText.innerText = "Disconnected";
                appendLog("\n--- Lost Connection to ESP32. Retrying... ---\n", "#f43f5e");
                setTimeout(connectWs, 3000);
            };
        }

        function sendTxPayload() {
            const text = cmdInput.value;
            if (ws && ws.readyState === WebSocket.OPEN && text.length > 0) {
                ws.send(text);
                appendLog(text, "#8b5cf6");
                cmdInput.value = "";
            }
        }

        function handleInputKey(event) {
            if (event.key === 'Enter') {
                sendTxPayload();
            }
        }

        function presetBaudChange() {
            const val = baudPresetSelect.value;
            if (val === "custom") {
                customBaudWrapper.style.display = "block";
            } else {
                customBaudWrapper.style.display = "none";
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send("CMD:BAUD:" + val);
                }
            }
        }

        function applyCustomBaud() {
            const customBaud = baudCustomInput.value;
            if (ws && ws.readyState === WebSocket.OPEN && customBaud > 0) {
                ws.send("CMD:BAUD:" + customBaud);
            }
        }

        function swapPins() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send("CMD:SWAP");
            }
        }

        function saveWifi() {
            const ssid = wifiSsidInput.value;
            const pass = wifiPassInput.value;
            if (ssid.length > 0) {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(`CMD:WIFI:${ssid}:${pass}`);
                    appendLog("\n[SYSTEM] Sent Wi-Fi config. ESP32 will save and reboot now...", "#10b981");
                }
            } else {
                alert("Please enter SSID!");
            }
        }

        connectWs();
    </script>
</body>
</html>
)rawliteral";

// Reconfigure Serial1 dynamically
void setupSerial1() {
  Serial1.end();
  Serial1.begin(baud_rate, SERIAL_8N1, pin_rx, pin_tx);
}

// Send current system state to all connected WS clients
void broadcastState() {
  String state = "STATE:" + String(pin_tx) + "," + 
                 String(pin_rx) + "," + 
                 String(baud_rate) + "," + 
                 String((WiFi.status() == WL_CONNECTED) ? "STA" : "AP") + "," + 
                 WiFi.localIP().toString();
  webSocket.broadcastTXT(state);
}

// Websocket Events Handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      {
        String state = "STATE:" + String(pin_tx) + "," + 
                       String(pin_rx) + "," + 
                       String(baud_rate) + "," + 
                       String((WiFi.status() == WL_CONNECTED) ? "STA" : "AP") + "," + 
                       WiFi.localIP().toString();
        webSocket.sendTXT(num, state);
      }
      break;
    case WStype_TEXT:
      {
        String text = String((char*)payload);
        if (text.startsWith("CMD:SWAP")) {
          pins_swapped = !pins_swapped;
          pin_rx = pins_swapped ? 17 : 16;
          pin_tx = pins_swapped ? 16 : 17;
          
          preferences.begin("bridge_config", false);
          preferences.putBool("swapped", pins_swapped);
          preferences.end();
          
          setupSerial1();
          broadcastState();
          Serial.printf("[SYSTEM] Swapped pins: TX=%d, RX=%d\n", pin_tx, pin_rx);
        }
        else if (text.startsWith("CMD:BAUD:")) {
          unsigned long new_baud = text.substring(9).toInt();
          if (new_baud > 0) {
            baud_rate = new_baud;
            
            preferences.begin("bridge_config", false);
            preferences.putUInt("baud", baud_rate);
            preferences.end();
            
            setupSerial1();
            broadcastState();
            Serial.printf("[SYSTEM] Changed Baud Rate: %lu\n", baud_rate);
          }
        }
        else if (text.startsWith("CMD:WIFI:")) {
          int second_colon = text.indexOf(':', 9);
          String ssid = text.substring(9, second_colon);
          String pass = text.substring(second_colon + 1);
          
          preferences.begin("bridge_config", false);
          preferences.putString("wifi_ssid", ssid);
          preferences.putString("wifi_pass", pass);
          preferences.end();
          
          Serial.println("[SYSTEM] Saved Wi-Fi credentials. Rebooting...");
          delay(1000);
          ESP.restart();
        }
        else {
          Serial1.print(text);
        }
      }
      break;
  }
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Starting Commandable Web UART Bridge ---");

  preferences.begin("bridge_config", true);
  wifi_ssid = preferences.getString("wifi_ssid", "");
  wifi_pass = preferences.getString("wifi_pass", "");
  baud_rate = preferences.getUInt("baud", 115200);
  pins_swapped = preferences.getBool("swapped", false);
  preferences.end();

  pin_rx = pins_swapped ? 17 : 16;
  pin_tx = pins_swapped ? 16 : 17;

  setupSerial1();
  Serial.printf("Configured UART: Baud: %lu | RX: GPIO %d | TX: GPIO %d\n", baud_rate, pin_rx, pin_tx);

  bool connected = false;
  if (wifi_ssid.length() > 0) {
    Serial.printf("Connecting to Wi-Fi SSID: '%s'...\n", wifi_ssid.c_str());
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    
    unsigned long start_time = millis();
    while (millis() - start_time < 30000) {
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
      delay(500);
      Serial.print(".");
    }
    Serial.println();
  }

  if (connected) {
    Serial.print("Wi-Fi connected successfully. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not configured or connection timed out. Falling back to Access Point Mode...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_UART_Bridge", "");
    Serial.print("Access Point created. AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP Web Server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket Server started");

  Serial.println("System Ready!");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  int available_bytes = Serial1.available();
  if (available_bytes > 0) {
    if (available_bytes > BUFFER_SIZE - 1) {
      available_bytes = BUFFER_SIZE - 1;
    }
    
    int bytes_read = Serial1.readBytes(rx_buf, available_bytes);
    rx_buf[bytes_read] = '\0';
    
    webSocket.broadcastTXT(rx_buf);
    Serial.write((uint8_t*)rx_buf, bytes_read);
  }

  if (Serial.available() > 0) {
    int usb_bytes = Serial.readBytes(rx_buf, sizeof(rx_buf));
    Serial1.write((uint8_t*)rx_buf, usb_bytes);
    
    rx_buf[usb_bytes] = '\0';
    webSocket.broadcastTXT(rx_buf);
  }
}
