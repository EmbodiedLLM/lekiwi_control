// http://192.168.4.1
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>
#include <SCServo.h>

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

NetworkServer server(80);

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
  sms_sts.WheelMode(7);
  sms_sts.WheelMode(9);
  sms_sts.WheelMode(8);
}

void loop() {
  NetworkClient client = server.accept();  // 监听客户端连接

  if (client) {                     // 如果有客户端连接
    Serial.println("New Client.");  // 打印连接信息
    String currentLine = "";        // 用于存储客户端请求的当前行
    while (client.connected()) {    // 当客户端保持连接时
      if (client.available()) {     // 如果有数据可读
        char c = client.read();     // 读取一个字节
        Serial.write(c);            // 打印到串口监视器
        if (c == '\n') {            // 如果是换行符

          // 如果当前行为空，表示请求结束，发送响应
          if (currentLine.length() == 0) {
            // HTTP 响应头
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.print("<!DOCTYPE html>");
            client.print("<html>");
            client.print("<head>");
            client.print("<title>Car Control</title>");
            client.print("<style>");
            client.print("h1 {");
            client.print("  text-align: center;"); /* 水平居中 */
            client.print("  font-size: 3em;");      /* 设置字体大小 (你可以调整) */
            client.print("  margin-bottom: 20px;"); /* 添加一些底部间距 */
            client.print("}");
            client.print("body {");
            client.print("  display: flex;");        /* 使用 Flexbox 布局 */
            client.print("  flex-direction: column;"); /* 垂直排列子元素 */
            client.print("  justify-content: center;"); /* 垂直居中子元素 */
            client.print("  align-items: center;");   /* 水平居中子元素 */
            client.print("  min-height: 100vh;");    /* 确保 body 占据整个视口高度 */
            client.print("  margin: 0;");           /* 移除默认的 body margin */
            client.print("}");
            client.print("</style>");
            client.print("</head>");
            client.print("<body>");

            // HTTP 响应内容
            client.print("<h1> <a href =\"/Forward\">Forward</a></h1>");
            client.print("<h1> <a href =\"/Backward\">Backward</a></h1>");
            client.print("<h1> <a href =\"/Turn_Left\">Turn Left</a></h1>");
            client.print("<h1> <a href =\"/Turn_Right\">Turn Right</a></h1>");
            client.print("<h1> <a href =\"/Stop\">Stop</a></h1>");
            client.print("<h1> <a href =\"/Go_Left\">Go Left</a></h1>");
            client.print("<h1> <a href =\"/Go_Right\">Go Right</a></h1>");



            client.print("</body>");
            client.print("</html>");


            // 响应结束
            client.println();
            break;
          } else {
            currentLine = "";  // 清空当前行
          }
        } else if (c != '\r') {  // 如果不是回车符
          currentLine += c;      // 添加到当前行
        }

        // 检查请求路径
        if (currentLine.endsWith("GET /Forward")) {
          Serial.println("Moving forward...");
          for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], Forward[i], ACC[i]);  // 设置舵机前进速度
          }
        }
        if (currentLine.endsWith("GET /Backward")) {
          Serial.println("Moving backward...");
          for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], -Forward[i], ACC[i]); // 设置舵机后退速度 (取反)
          }
        }
        // 30 degree per second
        if (currentLine.endsWith("GET /Turn_Right")) {
          Serial.println("Turning right...");
          for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], -450, ACC[i]); // 设置舵机后退速度 (取反)
          }
        }
        if (currentLine.endsWith("GET /Turn_Left")) {
          Serial.println("Turning left...");
          for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], 450, ACC[i]); // 设置舵机后退速度 (取反)
          }
        }
        if (currentLine.endsWith("GET /Stop")) {
          Serial.println("Stopping...");
          sms_sts.SyncWriteSpe(ID, 3, Stop, ACC); // 使用 SyncWriteSpe 停止所有舵机
        }

        if (currentLine.endsWith("GET /Go_Left")) {
          Serial.println("Go Left...");
            for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], go_left[i], ACC[i]); // go left
          }        
        }

        if (currentLine.endsWith("GET /Go_Right")) {
          Serial.println("Go Right...");
            for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], -go_left[i], ACC[i]); // go left
          }        
        }


      }
    }
    // 关闭客户端连接
    client.stop();
    Serial.println("Client Disconnected.");
  }
}