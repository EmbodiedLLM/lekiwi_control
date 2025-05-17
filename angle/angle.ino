#include <WiFi.h>
// #include <NetworkClient.h> // Likely WiFiClient, included by WiFi.h
// #include <WiFiAP.h>     // Likely WiFiServer, included by WiFi.h
#include <SCServo.h>
#include <math.h> // For cos() and sin()

SMS_STS sms_sts;

// set the default wifi mode here.
// 1 as [AP] mode, it will not connect other wifi.
// 2 as [STA] mode, it will connect to know wifi.
#define DEFAULT_WIFI_MODE 1

// the uart used to control servos.
// GPIO 18 - S_RXD, GPIO 19 - S_TXD, as default.
#define S_RXD 18
#define S_TXD 19


// Wi-Fi AP 配置
const char *ssid = "lekiwi";

WiFiServer server(80); // Use WiFiServer explicitly

// for positive 7 backward, 8 right, 9 forward
// 舵机配置
byte ID[3] = {7, 8, 9};
byte ACC[3] = {10, 10, 10};
s16 Forward[3] = {-1016, 0, 1016}; // 前进速度 0.1m/s
s16 Stop[3] = {0, 0, 0};           // 停止速度
s16 go_left[3] = {890, -1780, 890}; // 左平移速度 0.1m/s

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // 配置 Wi-Fi AP
  if (!WiFi.softAP(ssid)) {
    Serial.println("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");

  // 初始化舵机串口
  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  sms_sts.pSerial = &Serial1;
  delay(1000);

  // 设置舵机为速度模式
  // Ensure all relevant motors are in wheel mode.
  // Based on usage, ID[0](7), ID[1](8), ID[2](9) are used.
  for (int i = 0; i < 3; i++) {
    sms_sts.WheelMode(ID[i]);
    delay(10); // Small delay after mode change if necessary
  }
}

void loop() {
  WiFiClient client = server.accept();  // 监听客户端连接 (Use WiFiClient explicitly)

  if (client) {                     // 如果有客户端连接
    Serial.println("New Client.");
    String currentLine = "";        // 用于存储客户端请求的当前行 (HTTP request line)
    String requestHeader = "";      // To store other headers if needed, not strictly necessary for this simple case

    while (client.connected()) {    // 当客户端保持连接时
      if (client.available()) {     // 如果有数据可读
        char c = client.read();     // 读取一个字节
        // Serial.write(c);        // Optional: Echo all client communication to Serial Monitor
        
        if (c == '\n') { // End of a line from the client
          if (currentLine.length() == 0) { // An empty line signifies the end of HTTP request headers
            // We have received all headers, now send the HTTP response
            Serial.println("Sending HTTP response.");
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close"); // Advise client to close connection after response
            client.println(); // Blank line after headers

            // --- HTML Content ---
            client.print("<!DOCTYPE html>");
            client.print("<html>");
            client.print("<head>");
            client.print("<title>Car Control</title>");
            client.print("<style>");
            client.print("h1 { text-align: center; font-size: 3em; margin-bottom: 5px; }"); // Adjusted for more links
            client.print("body { justify-content: center; align-items: center; display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 100vh; margin: 0; font-family: Arial, sans-serif; }");
            client.print("a { text-decoration: none; color: #007bff; padding: 5px 0; } a:hover { color: #0056b3; }");
            client.print("form { text-align: center; margin-top: 20px; } input[type='text'] {padding: 8px; margin-right: 5px; border: 1px solid #ccc; border-radius: 4px;} input[type='submit'] {padding: 8px 15px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;} input[type='submit']:hover {background-color: #0056b3;}");
            client.print("</style>");
            client.print("</head>");
            client.print("<body>");

            client.print("<h1><a href=\"/Forward\">Forward</a></h1>");
            client.print("<h1><a href=\"/Backward\">Backward</a></h1>");
            client.print("<h1><a href=\"/Turn_Left\">Turn Left (Spin)</a></h1>");
            client.print("<h1><a href=\"/Turn_Right\">Turn Right (Spin)</a></h1>");
            client.print("<h1><a href=\"/Go_Left\">Strafe Left</a></h1>");
            client.print("<h1><a href=\"/Go_Right\">Strafe Right</a></h1>");
            client.print("<h1><a href=\"/Stop\">Stop</a></h1>");
            
            // Form for MoveAtAngle
            client.print("<div>"); // Using div for better structure if more form elements are added
            client.print("<h2>Move at Angle (0.1 m/s)</h2>");
            client.print("<form action='/MoveAtAngle' method='get'>");
            client.print("Angle (degrees): <input type='number' name='angle' min='0' max='360' step='any' value='0'>");
            client.print("<input type='submit' value='Go'>");
            client.print("</form>");
            client.print("</div>");

            client.print("</body>");
            client.print("</html>");
            client.println(); // Final blank line for HTTP response
            
            break; // Break out of the while loop (reading client data)
            
          } else { // It's a header line (or the initial request line)
            if (currentLine.startsWith("GET ")) { // This is the HTTP GET request line
              Serial.print("Request: ");
              Serial.println(currentLine);

              // --- Command Parsing ---
              if (currentLine.startsWith("GET /Forward ")) {
                Serial.println("Action: Moving forward...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], Forward[i], ACC[i]);
                }
              } else if (currentLine.startsWith("GET /Backward ")) {
                Serial.println("Action: Moving backward...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], (s16)-Forward[i], ACC[i]);
                }
              } else if (currentLine.startsWith("GET /Turn_Right ")) {
                Serial.println("Action: Turning right (spin)...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], -450, ACC[i]); // All wheels same direction for spin
                }
              } else if (currentLine.startsWith("GET /Turn_Left ")) {
                Serial.println("Action: Turning left (spin)...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], 450, ACC[i]); // All wheels same direction for spin
                }
              } else if (currentLine.startsWith("GET /Stop ")) {
                Serial.println("Action: Stopping...");
                sms_sts.SyncWriteSpe(ID, 3, Stop, ACC);
              } else if (currentLine.startsWith("GET /Go_Left ")) {
                Serial.println("Action: Strafing Left...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], go_left[i], ACC[i]);
                }
              } else if (currentLine.startsWith("GET /Go_Right ")) {
                Serial.println("Action: Strafing Right...");
                for (int i = 0; i < 3; i++) {
                  sms_sts.WriteSpe(ID[i], (s16)-go_left[i], ACC[i]);
                }
              } else if (currentLine.startsWith("GET /MoveAtAngle?angle=")) {
                Serial.println("Action: Move at specific angle...");
                int indexOfEquals = currentLine.indexOf('=');
                int indexOfSpace = currentLine.indexOf(' ', indexOfEquals); // Look for ' HTTP/1.1'
                String angleStr;

                if (indexOfEquals != -1) { // Check if '=' is found
                  if (indexOfSpace > indexOfEquals) {
                    angleStr = currentLine.substring(indexOfEquals + 1, indexOfSpace);
                  } else {
                    // Fallback if no space (e.g. parameter is at the very end of the line before HTTP version)
                    // Or if the HTTP version part is missing (less robust client)
                    angleStr = currentLine.substring(indexOfEquals + 1);
                  }
                  
                  float angle_degrees = angleStr.toFloat();
                  Serial.print("Requested angle (degrees): ");
                  Serial.println(angle_degrees);

                  float angle_radians = angle_degrees * PI / 180.0;

                  s16 s7_calc = (s16)(-1016.0 * cos(angle_radians) - 890.0 * sin(angle_radians));
                  s16 s8_calc = (s16)(1780.0 * sin(angle_radians));
                  s16 s9_calc = (s16)(1016.0 * cos(angle_radians) - 890.0 * sin(angle_radians));

                  Serial.print("Calculated servo speeds: s7=");
                  Serial.print(s7_calc);
                  Serial.print(", s8=");
                  Serial.print(s8_calc);
                  Serial.print(", s9=");
                  Serial.println(s9_calc);

                  sms_sts.WriteSpe(ID[0], s7_calc, ACC[0]); // Servo 7
                  sms_sts.WriteSpe(ID[1], s8_calc, ACC[1]); // Servo 8
                  sms_sts.WriteSpe(ID[2], s9_calc, ACC[2]); // Servo 9
                } else {
                  Serial.println("Error: Malformed MoveAtAngle command. '=' not found.");
                }
              }
            } // End of GET checks
            currentLine = ""; // Clear currentLine to read the next header line
          }
        } else if (c != '\r') { // If you got anything else but a carriage return (and not newline)
          currentLine += c;      // Add it to the end of the currentLine
          if (currentLine.length() > 255) { // Prevent buffer overflow for currentLine
            Serial.println("Error: Request line too long.");
            currentLine = ""; // Reset
          }
        }
      } // End if client.available()
    } // End while client.connected()
    
    // Close the connection with the client
    client.stop();
    Serial.println("Client Disconnected.");
  } // End if (client)
}