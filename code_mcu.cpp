#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>
#define MQTT_USER "admin"
#define MQTT_PASS "123"
// Topics
#defineTOPIC_SET_TEMP"esp32/set/tem"     // setpoint và nhiệt độ hiện tại
#define TOPIC_SET_TIME "esp32/set/tim"     // thời gian
#define TOPIC_SET_STATUS "esp32/set/status"
#define TOPIC_FTIM "esp32/FTim"
#define TOPIC_GET_C_TEMP "esp32/get/C_temp"
#define TOPIC_GET_TIME "esp32/get/tim"   // thời gian
#define TOPIC_GET_TEMP "esp32/get/tem"
#define TOPIC_GET_PWM "esp32/get/pwm"
#define TOPIC_GET_STATUS "esp32/get/status"
// ========= LCD =========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== Encoders & Buttons =========
#define ENC1_CLK 18
#define ENC1_DT 19
#define ENC1_SW 23
#define ENC2_CLK 4
#define ENC2_DT 5
#define ENC2_SW 25
#define Button 27 
// ========= Outputs =========
#define RELAY_IN 26
#define OUT1 32
#define OUT2 13
// ========= Sensor DS18B20 =========
#define SENSOR_PIN 17
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// ========= PID =========
// ========= MQTT Clients =========
WiFiClient espClient;
PubSubClient mqtt(espClient);
// ====== Debounce buttons =========
unsigned long lastBtnCheck = 0;
const unsigned long BTN_DEBOUNCE_MS = 50;
int lastBtn1 = HIGH, lastBtn2 = HIGH;
volatile unsigned long lastEnc1Tick = 0;
volatile unsigned long lastEnc2Tick = 0;
float lasttimcheck=0;
const unsigned long ENCODER_DEBOUNCE_US = 1000; // 1ms

// ========= Utils =========
void lcdShow()
{
  char l1[17], l2[17];
  // Dòng 1: Setpoint + nhiệt độ hiện tại
  snprintf(l1, sizeof(l1), "SP:%2dC NH:%4.1f", tem, Input);
  // Dòng 2: TIME + ON/OFF + WiFi status
  const char *st = (status == 1) ? "ON " : "OFF";
  const char *wifiSt = (WiFi.status() == WL_CONNECTED) ? "WF:OK" : "WF:ER";
  snprintf(l2, sizeof(l2), "T:%0.1f %s %s", tim, st, wifiSt);

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// Gửi topic khi giá trị thay đổi
// Gửi countdown ftim mỗi giây
void publishFtim() {
  unsigned long now = millis();
  if (now - lastFtimPublish >= 1000) {
    lastFtimPublish = now;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", ftim);
    mqtt.publish(TOPIC_FTIM, buf, true);
  }
}
void wifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  char msg[32];
  unsigned int n = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;
  memcpy(msg, payload, n);
  msg[n] = '\0';
  if (strcmp(topic, TOPIC_SET_TEMP) == 0)
  {
    int newTem = constrain(atoi(msg), 0, 80);
    if (newTem != tem) { // chỉ cập nhật nếu khác
      tem = newTem;
      if (status == 1 && tim > 0) {
            ftime = millis() + (unsigned long)(tim * 60 * 60 * 1000);
        }
    }
  }
if (!mqtt.connected())
  {
    String cid = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASS))
    {
      mqtt.subscribe(TOPIC_SET_TEMP);
      mqtt.subscribe(TOPIC_SET_TIME);
      mqtt.subscribe(TOPIC_SET_STATUS);
      //publishAll();
    }
    else
    {
      delay(2000);
    }
  }
}
// ========= ISRs =========
void IRAM_ATTR readEncoder1()
{
  unsigned long now = micros();
  if (now - lastEnc1Tick > ENCODER_DEBOUNCE_US) {
    int dtValue = digitalRead(ENC1_DT);
    enc1Count += (dtValue == HIGH) ? 1 : -1;
    lastEnc1Tick = now;
  }
}
void IRAM_ATTR readEncoder2()
{
   unsigned long now = micros();
  if (now - lastEnc2Tick > ENCODER_DEBOUNCE_US) {
    int dtValue = digitalRead(ENC2_DT);
    enc2Count += (dtValue == HIGH) ? 1 : -1;
    lastEnc2Tick = now;
  }
}
ledcWrite(pwmChannel1, 0);
  ledcWrite(pwmChannel2, 0);
  // Sensor
  sensors.begin();
  // WiFi + MQTT
  wifiConnect();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  lcdShow();
}
// ========= Loop =========
void loop()
{
  if (!mqtt.connected())
    mqttReconnect();
  mqtt.loop();
  unsigned long now = millis();
  // Encoder xử lý
  static int oldEnc1 = 0, oldEnc2 = 0;
  if (enc1Count != oldEnc1)
  {
    tem = constrain(tem + (enc1Count - oldEnc1), 0, 60);
    oldEnc1 = enc1Count;
    lcdShow();
  }
  if (enc2Count != oldEnc2)
  {
    tim = constrain(tim + (enc2Count - oldEnc2), 0, 50);
    oldEnc2 = enc2Count;
    ftime = millis() + (unsigned long)(tim * 60 * 60 * 1000);
    lcdShow();
  }
  // Nút ON/OFF debounce
  if (digitalRead(Button_2) == LOW)
  { 
ledcWrite(pwmChannel2, 0);
    integral = 0.0;
  }
  // PID + đọc nhiệt độ mỗi 1s
  if (now - lastTempTick >= TEMP_INTERVAL_MS)
  {
    lastTempTick = now;
    sensors.requestTemperatures();
    Input = sensors.getTempCByIndex(0);
    if (Input == DEVICE_DISCONNECTED_C)
    {
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!   ");
      digitalWrite(RELAY_IN, LOW);
      ledcWrite(pwmChannel1, 0);
      status = -1;
      return;
    }
    if (status == 1)
    {
      Setpoint = tem;
      double error = Setpoint - Input;
      integral += error * (TEMP_INTERVAL_MS / 1000.0);
// ========= WiFi & MQTT =========
#define WIFI_SSID "Dong 4 Sau"
#define WIFI_PASS "dong146779"
#define MQTT_SERVER "192.168.100.154" // IP máy chạy Node-RED (Aedes)
#define MQTT_PORT 1883
ouble Kp = 330, Ki = 0.9, Kd = 0.1;
double Setpoint = 25.0, Input = 0.0, Output = 0.0;
double integral = 0.0, previousError = 0.0;

// ========= Vars =========
int tem = 25;                      // setpoint °C
float tim = 0;                     // time
int status = -1;                   // 1=ON, -1=OFF
int duty = 0;                      // duty cycle
            // trạng thái PID đang chạy hay không
int cu_time = 0;                   // thời gian hiện tại
unsigned long ftime = 0, ftim = 0; // thời gian kết thúc
unsigned long lastTempTick = 0;
const unsigned long TEMP_INTERVAL_MS = 750;


// Biến lưu giá trị trước đó để so sánh
int lastTem = -1;
float lastTim = -1;
int lastDuty = -1;
int lastStatus = -2; // khởi tạo khác status
float lastTimeSet = -1;   // lưu tim
float lastInputTemp = -1; // lưu Input
unsigned long lastFtimPublish = 0;


// ========= Encoder counters =========
volatile int enc1Count = 0;
volatile int enc2Count = 0;

// ========= PWM =========
const int pwmFreq = 750;
const int pwmResolution = 8;
const int pwmChannel1 = 0;
const int pwmChannel2 = 1;
void publishIfChanged() {
  char buf[16];

  if (tem != lastTem) {
    snprintf(buf, sizeof(buf), "%d", tem);
    mqtt.publish(TOPIC_GET_TEMP, buf, true);
    lastTem = tem;
  }

  if (Input != lastInputTemp) { // nếu muốn gửi nhiệt độ hiện tại riêng
    snprintf(buf, sizeof(buf), "%.1f", Input);
    mqtt.publish(TOPIC_GET_C_TEMP, buf, true);
    lastInputTemp = Input;
  }

  if (duty != lastDuty) {
    snprintf(buf, sizeof(buf), "%d", duty);
    mqtt.publish(TOPIC_GET_PWM, buf, true);
    lastDuty = duty;
  }

  if (status != lastStatus) {
    snprintf(buf, sizeof(buf), "%d", status);
    mqtt.publish(TOPIC_GET_STATUS, buf, true);
    lastStatus = status;
  }

  if (tim != lastTimeSet) {
    snprintf(buf, sizeof(buf), "%.2f", tim);
    mqtt.publish(TOPIC_GET_TIME, buf, true);
    lastTimeSet = tim;
  }
}
else if (strcmp(topic, TOPIC_SET_TIME) == 0)
  {
    float newTim = constrain(atof(msg), 0, 99);
    if (newTim != tim) { // chỉ cập nhật nếu khác
      tim = newTim;
      if (status == 1 && tim > 0) {
        ftime = millis() + (unsigned long)(tim * 60 * 60 * 1000);
      }
    }
  }
  else if (strcmp(topic, TOPIC_SET_STATUS) == 0)
  {
    int s = atoi(msg);
    if (s > 0 && status != 1)
    { // chuyển sang ON
      status = 1;
      if (tim > 0)
        ftime = millis() + (unsigned long)(tim * 60 * 60 * 1000);
    }
    else if (s <= 0 && status != -1)
    { // chuyển sang OFF
      status = -1;
      
      duty = 0;
      digitalWrite(RELAY_IN, LOW);
      ledcWrite(pwmChannel1, duty);
      ledcWrite(pwmChannel2, duty);
      integral = 0.0;
    }
  }
  //publishAll();
  lcdShow();
}
void mqttReconnect()
{
// ========= Setup =========
void setup()
{
  Serial.begin(115200);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");

  // Encoders
  pinMode(ENC1_CLK, INPUT_PULLUP);
  pinMode(ENC1_DT, INPUT_PULLUP);
  pinMode(ENC1_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC1_CLK), readEncoder1, FALLING);
  pinMode(ENC2_CLK, INPUT_PULLUP);
  pinMode(ENC2_DT, INPUT_PULLUP);
  pinMode(ENC2_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC2_CLK), readEncoder2, FALLING);
  // Buttons
  pinMode(Button, INPUT_PULLUP);
  // Outputs
  pinMode(RELAY_IN, OUTPUT);
  pinMode(OUT1, OUTPUT);
  digitalWrite(RELAY_IN, LOW);
  digitalWrite(OUT1, LOW);
  // PWM
  ledcSetup(pwmChannel1, pwmFreq, pwmResolution);
  ledcSetup(pwmChannel2, pwmFreq, pwmResolution);
  ledcAttachPin(OUT2, pwmChannel2);
  ledcAttachPin(OUT1, pwmChannel1);
if (status == -1)
    {
      status = 1;
    }
    else
    {
      status = -1;
      digitalWrite(RELAY_IN, LOW);
      ledcWrite(pwmChannel1, 0);
      ledcWrite(pwmChannel2, 0);
      integral = 0.0;
      lcdShow();
    }
  }
  // kiểm tra ftim
  static unsigned long lastFtimTick = 0;
  if (status == 1 && tim >= 0 && millis() - lastFtimTick >= 1000)
  { 
    if (tim != lasttimcheck ){
        lasttimcheck = tim;
        ftime = millis() + (unsigned long)(tim * 60 * 60 * 1000);    
    }
    lastFtimTick = millis();
    long remaining = (long)(ftime - millis());
    if (remaining < 0)
      remaining = 0;
    ftim = remaining / 1000; // giây còn lại
    //publishAll();
  }
  // kiểm tra hết giờ
  if (status == 1 && tim >= 0 && millis() >= ftime)
  { ftim = 0;
    status = -1;
    digitalWrite(RELAY_IN, LOW);
    ledcWrite(pwmChannel1, 0);
integral = constrain(integral, -100, 100);
      double derivative = (error - previousError) / (TEMP_INTERVAL_MS / 1000.0);
      Output = Kp * error + Ki * integral + Kd * derivative;
      previousError = error;
      duty = constrain((int)Output, 0, 255);
      digitalWrite(RELAY_IN, (Output > 0));
      ledcWrite(pwmChannel1, duty);
      ledcWrite(pwmChannel2, duty);
    }
    else
    {
      digitalWrite(RELAY_IN, LOW);
      duty = 0;
      ledcWrite(pwmChannel1, duty);
      ledcWrite(pwmChannel2, duty);
      integral = 0.0;
    }
    lcdShow();
  }
  // Cuối loop, gửi topic chỉ khi cần
  publishIfChanged();
  publishFtim();
  delay(5); // Giảm tải CPU
}