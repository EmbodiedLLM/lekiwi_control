// http://192.168.4.1/

#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>
#include <SCServo.h>

SMS_STS sms_sts;

#define S_RXD 4    // 设置串口接收引脚
#define S_TXD 6    // 设置串口发送引脚

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // 如果开发板没有内置 LED，请设置测试 LED 的 GPIO 引脚
#endif

// Wi-Fi AP 配置
const char *ssid = "lekiwi";

NetworkServer server(80);

// 舵机配置
byte ID[3] = {7, 8, 9};
byte ACC[3] = {10, 10, 10};
s16 Forward[3] = {261, -521, 261}; // 前进速度
s16 Stop[3] = {0, 0, 0};           // 停止速度

void setup() {
  // 初始化 LED
  pinMode(LED_BUILTIN, OUTPUT);

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

            // HTTP 响应内容
            client.print("Click <a href=\"/Forward\">here</a> to move forward.<br>");
            client.print("Click <a href=\"/Stop\">here</a> to stop the car.<br>");

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
          digitalWrite(LED_BUILTIN, HIGH);  // 打开 LED 作为指示
        }
        if (currentLine.endsWith("GET /Stop")) {
          Serial.println("Stopping...");
          for (int i = 0; i < 3; i++) {
            sms_sts.WriteSpe(ID[i], Stop[i], ACC[i]);  // 设置舵机停止
          }
          digitalWrite(LED_BUILTIN, LOW);  // 关闭 LED 作为指示
        }
      }
    }
    // 关闭客户端连接
    client.stop();
    Serial.println("Client Disconnected.");
  }
}