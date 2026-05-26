#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// ==========================================
//            CONFIGURATION V2
// ==========================================
#define SIMULATE_LPC_HARDWARE false 

const char* ssid = "BMS_Dashboard";
const char* password = "admin123";

#define I2C_SDA 21
#define I2C_SCL 22
#define INA226_ADDR 0x40 

#define PIN_TAP1 39 // VN 
#define PIN_TAP2 34 // D34
#define PIN_TAP3 35 // D35
#define PIN_NTC  36 // VP 

#define PIN_BAL1    25 
#define PIN_BAL2    26 
#define PIN_BAL3    27 
#define PIN_RELAY   13 // Safety cut-off relay output
// ==========================================

WebServer server(80);

String latestJsonStr = "{\"V\":0.0,\"I\":0.0,\"P\":0.0,\"C1\":0.0,\"C2\":0.0,\"C3\":0.0,\"NTC\":0.0,\"R\":0,\"B\":[0,0,0]}";

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS Telemetry HUD</title>
  <style>
    body { background: #060b13; color: #f1f5f9; font-family: 'Courier New', Courier, monospace; display: flex; flex-direction: column; align-items: center; padding: 20px;}
    .header { font-size: 24px; letter-spacing: 2px; color: #94a3b8; margin-bottom: 20px; align-self: flex-start; margin-left: 5%;}
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 15px; max-width: 1200px; width: 100%; }
    .panel { background: rgba(15, 23, 42, 0.7); border: 1px solid #334155; border-radius: 4px; padding: 20px; }
    .title { font-size: 12px; text-transform: uppercase; letter-spacing: 1px; color: #64748b; margin-bottom: 15px; border-bottom: 1px solid #1e293b; padding-bottom: 5px;}
    .bignum { font-size: 42px; font-weight: bold; display: flex; justify-content: space-between; align-items: baseline;}
    .bignum span { font-size: 14px; color: #64748b; font-family: sans-serif; letter-spacing: 2px;}
    .green { color: #22c55e; }
    .red { color: #ef4444; }
    .orange { color: #eab308; }
    
    .relay-btn { font-size: 24px; font-weight: bold; padding: 20px; text-align: center; border-radius: 4px; border: 2px solid; margin-bottom: 25px; font-family: 'Segoe UI', Tahoma, sans-serif;}
    .relay-ok { color: #22c55e; border-color: #22c55e; box-shadow: 0 0 15px rgba(34,197,94,0.15), inset 0 0 15px rgba(34,197,94,0.05); }
    .relay-err { color: #ef4444; border-color: #ef4444; box-shadow: 0 0 20px rgba(239,68,68,0.4), inset 0 0 20px rgba(239,68,68,0.2); animation: blink 1s infinite alternate; }
    @keyframes blink { 100% { opacity: 0.7; box-shadow: none; } }
    
    .cells { display: flex; justify-content: space-around; height: 160px; margin-top: 40px; margin-bottom: 10px;}
    .bat { width: 45px; border: 2px solid #475569; border-radius: 3px; position: relative; background: rgba(0,0,0,0.5); display: flex; flex-direction: column; justify-content: flex-end; align-items: center; transition: all 0.3s;}
    .bat:before { content:''; position: absolute; top:-6px; width:16px; height:4px; background:#475569; border-radius: 2px 2px 0 0; }
    .fill { width: 100%; height: 0%; background: linear-gradient(0deg, #15803d, #4ade80); transition: height 0.5s ease-out; border-radius: 0 0 2px 2px;}
    .bat-label { position: absolute; bottom: 100%; padding-bottom: 10px; text-align: center; font-size: 14px;}
    .bleed-text { position: absolute; top:100%; margin-top:10px; font-size:11px; color:#eab308; opacity:0; letter-spacing: 1px; font-weight: bold;}
    
    .is-bleeding .bleed-text { opacity:1; }
    .is-bleeding { border-color:#eab308; box-shadow: 0 0 10px rgba(234, 179, 8, 0.4); }
    
    .logs { line-height: 2; font-size: 14px; background: rgba(0,0,0,0.3); padding: 15px; border-radius: 4px;}
  </style>
</head>
<body>
  <div class="header">3S BATTERY MANAGEMENT SYSTEM - NODE 1</div>
  <div class="grid">
    
    <!-- PANEL 1 -->
    <div class="panel">
      <div class="title">Primary Telemetry</div>
      
      <div class="cells">
        <div class="bat" id="bat1"><div class="bat-label">CELL 1<br><span id="c1v">--</span></div><div class="fill" id="fill1"></div><div class="bleed-text">BLEEDING</div></div>
        <div class="bat" id="bat2"><div class="bat-label">CELL 2<br><span id="c2v">--</span></div><div class="fill" id="fill2"></div><div class="bleed-text">BLEEDING</div></div>
        <div class="bat" id="bat3"><div class="bat-label">CELL 3<br><span id="c3v">--</span></div><div class="fill" id="fill3"></div><div class="bleed-text">BLEEDING</div></div>
      </div>

      <div class="title" style="margin-top:30px;">PACK VOLTAGE</div>
      <div class="bignum" id="v">--<span>VOLTS</span></div>
      
      <div class="title" style="margin-top:20px;">LIVE CURRENT</div>
      <div class="bignum" id="i"><span class="green" style="font-size:36px;font-family:monospace;letter-spacing:0;" id="i_val">--</span><span>AMPS</span></div>

      <div class="title" style="margin-top:20px;">LIVE POWER</div>
      <div class="bignum" id="p">--<span>WATTS</span></div>
    </div>
    
    <!-- PANEL 2 -->
    <div class="panel">
      <div class="title">Advanced Analytics</div>
      
      <div class="title" style="border:none; text-align:center;">STATE OF CHARGE (SOC)</div>
      <div class="bignum" id="soc" style="justify-content:center; color:#4ade80; font-size: 60px; margin: 30px 0;">--%</div>
      
      <div class="title" style="margin-top:40px;">CELL IMBALANCE (&Delta;V)</div>
      <div class="bignum" id="deltav">--<span>VOLTS</span></div>

      <div class="title" style="margin-top:40px;">THERMALS (NTC)</div>
      <div class="bignum" id="temp">--<span>&deg;C</span></div>
    </div>

    <!-- PANEL 3 -->
    <div class="panel">
      <div class="title">System Status</div>
      <div class="relay-btn" id="relay">CONNECTING...</div>
      
      <div class="title">Balancing Routine</div>
      <div class="logs">
        C1 STATUS: <span id="bal1">--</span><br>
        C2 STATUS: <span id="bal2">--</span><br>
        C3 STATUS: <span id="bal3">--</span>
      </div>
      
      <div class="title" style="margin-top:30px;">MCU UPTIME</div>
      <div class="logs" id="uptime">0s</div>
    </div>

  </div>

  <script>
    let loadTime = Date.now();
    setInterval(()=> {
        document.getElementById('uptime').innerText = Math.floor((Date.now() - loadTime)/1000) + "s";
    }, 1000);

    function update() {
      fetch('/api/data').then(r=>r.json()).then(data => {
        document.getElementById('v').innerHTML = data.V.toFixed(2) + '<span>VOLTS</span>';
        
        let iStr = data.I.toFixed(2);
        if(data.I > 0) iStr = "+" + iStr; 
        document.getElementById('i_val').innerText = iStr;
        
        document.getElementById('p').innerHTML = data.P.toFixed(1) + '<span>WATTS</span>';
        document.getElementById('temp').innerHTML = data.NTC.toFixed(1) + '<span>&deg;C</span>';
        
        // Relay Logig (1 is logic HIGH / safe)
        const r = document.getElementById('relay');
        if(data.R) { 
            r.className="relay-btn relay-ok"; r.innerHTML="RELAY STATE:<br>CONNECTED"; 
        } else { 
            r.className="relay-btn relay-err"; r.innerHTML="RELAY STATE:<br>FAULT TRIPPED"; 
        }
        
        // Battery Rendering
        const updateBat = (id, v, isBal) => {
          document.getElementById('c'+id+'v').innerText = v.toFixed(2)+'V';
          // Fill math: 3.0V is 0%, 4.2V is 100%
          let pct = ((v - 3.0) / (4.2 - 3.0)) * 100;
          if(pct < 0) pct=0; if(pct>100) pct=100;
          document.getElementById('fill'+id).style.height = pct+'%';
          
          const bE = document.getElementById('bat'+id);
          const stat = document.getElementById('bal'+id);
          
          if(isBal) { 
              bE.classList.add('is-bleeding'); 
              stat.innerText = "ACTIVE"; 
              stat.className="orange"; 
          } else { 
              bE.classList.remove('is-bleeding'); 
              stat.innerText = "IDLE"; 
              stat.className="green"; 
          }
        };
        updateBat(1, data.C1, data.B[0]);
        updateBat(2, data.C2, data.B[1]);
        updateBat(3, data.C3, data.B[2]);
        
        // Analytics
        let max = Math.max(data.C1, data.C2, data.C3);
        let min = Math.min(data.C1, data.C2, data.C3);
        document.getElementById('deltav').innerHTML = (max-min).toFixed(2) + '<span>VOLTS</span>';
        
        // SOC Math
        let soc = ((data.V - 9.0) / (12.6 - 9.0)) * 100;
        if(soc < 0) soc=0; if(soc>100) soc=100;
        document.getElementById('soc').innerText = soc.toFixed(0) + '%';
      });
    }
    setInterval(update, 1000); update();
  </script>
</body>
</html>)rawliteral";

uint16_t readINA226Register(uint8_t reg) {
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) return 0;
  
  Wire.requestFrom(INA226_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    uint16_t res = (Wire.read() << 8) | Wire.read();
    return res;
  }
  return 0;
}

void handleRoot() { server.send(200, "text/html", index_html); }
void handleApi() { server.send(200, "application/json", latestJsonStr); }

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  if(SIMULATE_LPC_HARDWARE) {
    Wire.begin(I2C_SDA, I2C_SCL);
    pinMode(PIN_BAL1, OUTPUT); pinMode(PIN_BAL2, OUTPUT); pinMode(PIN_BAL3, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, HIGH); // Default to safely connected
    Serial.println("\n*** HARDWARE SIMULATION MODE ENABLED ***");
  }

  Serial.print("Starting AP... ");
  if (WiFi.softAP(ssid, password)) {
    Serial.print("http://"); Serial.println(WiFi.softAPIP());
  }

  server.on("/", handleRoot);
  server.on("/api/data", handleApi);
  server.begin();
}

unsigned long lastSimTime = 0;

void loop() {
  server.handleClient();

  if (SIMULATE_LPC_HARDWARE) {
    if (millis() - lastSimTime > 1000) {
      lastSimTime = millis();
      
      float pack_v = readINA226Register(0x02) * 0.00125;
      float current = ((int16_t)readINA226Register(0x01) * 0.0000025) / 0.01;
      float power = pack_v * current; // LIVE POWER !

      // PRECISE Calibrated Multipliers (Target: 4.11, 4.12, 4.09)
      float CAL_1 = 1.0371; 
      float CAL_2 = 1.0541; 
      float CAL_3 = 0.9957; 

      // IMPORTANT: Turn OFF all balancers BEFORE reading ADC to prevent electrical noise feedback!
      digitalWrite(PIN_BAL1, LOW);
      digitalWrite(PIN_BAL2, LOW);
      digitalWrite(PIN_BAL3, LOW);
      delayMicroseconds(500); // Let the breadboard settle

      // OVERSAMPLING: Take 10 rapid readings instantaneously to physically cancel thermal noise
      auto getStableRead = [](int pin) {
          int sum = 0;
          for(int i = 0; i < 10; i++) sum += analogRead(pin);
          return sum / 10.0;
      };

      float tap1 = (getStableRead(PIN_TAP1) * (3.3 / 4095.0)) * 1.4769 * CAL_1;
      float tap2 = (getStableRead(PIN_TAP2) * (3.3 / 4095.0)) * 3.2359 * CAL_2;
      float tap3 = (getStableRead(PIN_TAP3) * (3.3 / 4095.0)) * 4.0461 * CAL_3;
      
      float ntc_v = analogRead(PIN_NTC) * (3.3 / 4095.0);
      float temp_c = 0.0;
      if (ntc_v < 3.2 && ntc_v > 0.1) { 
        float r_ntc = 10000.0 * ntc_v / (3.3 - ntc_v);
        temp_c = 1.0 / (1.0 / (25.0 + 273.15) + (1.0 / 3950.0) * log(r_ntc / 10000.0)) - 273.15;
      }
      
      float c1 = tap1; float c2 = tap2 - tap1; float c3 = tap3 - tap2;
      if (c2 < 0) c2 = 0; if (c3 < 0) c3 = 0;
      
      int relayState = 1;
      if(current > 8.0 || pack_v > 12.7 || pack_v < 8.5) relayState = 0; 
      
      digitalWrite(PIN_RELAY, relayState ? HIGH : LOW);
      
      // DISCHARGE INTERLOCK: Only balance if the battery is NOT actively powering a heavy load.
      // Assuming positive current means Discharge. If you wire INA226 backwards, flip this!
      bool isDischarging = (current > 0.15); // Triggered if pulling more than 150mA

      int bal1 = (c1 > 4.15 && !isDischarging) ? 1 : 0;
      int bal2 = (c2 > 4.15 && !isDischarging) ? 1 : 0;
      int bal3 = (c3 > 4.15 && !isDischarging) ? 1 : 0;
      
      // NOW turn balancers back on AFTER the ADC reads are done
      digitalWrite(PIN_BAL1, bal1 ? HIGH : LOW);
      digitalWrite(PIN_BAL2, bal2 ? HIGH : LOW);
      digitalWrite(PIN_BAL3, bal3 ? HIGH : LOW);

      char jsonBuf[200];
      sprintf(jsonBuf, "{\"V\":%.2f,\"I\":%.2f,\"P\":%.2f,\"C1\":%.2f,\"C2\":%.2f,\"C3\":%.2f,\"NTC\":%.2f,\"R\":%d,\"B\":[%d,%d,%d]}",
              pack_v, current, power, c1, c2, c3, temp_c, relayState, bal1, bal2, bal3);
      latestJsonStr = String(jsonBuf);
      
      Serial.println(latestJsonStr);
    }
  } else {
    // UART Pass-through Mode
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n'); line.trim();
      if(line.startsWith("{") && line.endsWith("}")) latestJsonStr = line;
    }
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n'); line.trim();
      if(line.startsWith("{") && line.endsWith("}")) latestJsonStr = line;
    }
  }
}
