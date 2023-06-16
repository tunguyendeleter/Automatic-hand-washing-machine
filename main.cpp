#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "FirebaseESP32.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>

#define trig 26 // chân trig của HC-SR04
#define echo 27 // chân echo của HC-SR04
#define C51_INPUT 5
#define FIREBASE_HOST "https://rtos-62f4e-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "7nfBZFeoyZKZlTvwtfHPpdtlMMLks6Wi21gcE1KC"
#define BIT_0 (1 << 0) // firebase
#define BIT_1 (1 << 1) // servo
#define BIT_2 (1 << 2) // temp > 34
#define BIT_3 (1 << 3) // temp <= 34
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo; // create servo object to control a servo
static const BaseType_t app_cpu0 = 0;
static const BaseType_t app_cpu1 = 1;

QueueHandle_t xQueueAmbient, xQueueObject, xQueueVar;
EventGroupHandle_t xEventGroup;
SemaphoreHandle_t xSemaphore = NULL;
// TaskHandle_t xHandle0, xHandle1, xHandle2,
xHandle3, xHandle4;
FirebaseData firebaseData;
FirebaseJson json;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

EventBits_t eventBits;
static int servoPin = 13;
const char *ssid = "iphonet";
const char *password = "truonghero100000";
String RTDB = "/ESP32";
String RoomTemp = "/Room_Temp";
String ObjectTemp = "/Object_Temp";
bool var = true;
void printValue(void *parameter)
{
  while (1)
  {
    int Ambient = mlx.readAmbientTempC();
    int Object = mlx.readObjectTempC();
    Serial.print("Ambient = ");
    Serial.print(Ambient);
    Serial.print("*C\tObject = ");
    Serial.print(Object);
    Serial.println("*C");
    Serial.println();
    xQueueOverwrite(xQueueAmbient, &Ambient);
    xQueueOverwrite(xQueueObject, &Object);
    xSemaphoreGive(xSemaphore);
    eventBits = xEventGroupSetBits(xEventGroup, BIT_0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
void getValue(void *parameter)
{
  while (1)
  {
    int Ambient;
    int Object;
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE)
    {
      if (xQueueAmbient != 0)
      {
        if (xQueuePeek(xQueueAmbient, &(Ambient), (TickType_t)10))
        {
          Firebase.setInt(firebaseData, RTDB + RoomTemp, Ambient);
        }
      }
      if (xQueueObject != 0)
      {
        if (xQueuePeek(xQueueObject, &(Object), (TickType_t)10))
        {
          Firebase.setInt(firebaseData, RTDB + ObjectTemp, Object);
        }
      }
    }
  }
}
void lcdControl(void *parameter)
{
  while (1)
  {
    eventBits = xEventGroupWaitBits(
        xEventGroup,               /* The event group being tested. */
        BIT_0,                     /* The bits within the event group to wait for. */
        pdFALSE,                   /* BIT_0 should be cleared before returning. */
        pdFALSE,                   /* Don't wait for both bits, either bit will do. */
        100 / portTICK_PERIOD_MS); /* Wait a maximum of 100ms for either bit to be set. */
    if ((eventBits & BIT_0) != 0)
    {
      int Object;
      xQueuePeek(xQueueObject, &(Object), (TickType_t)10);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print(Object);
      lcd.print(" *C  ");
      lcd.setCursor(0, 1);
      if (Object >= 34)
      {
        lcd.print(" You are Sick  ");
        eventBits = xEventGroupSetBits(xEventGroup, BIT_2);
      }
      else
      {
        lcd.print(" You are fine  ");
        eventBits = xEventGroupSetBits(xEventGroup, BIT_3);
      }
      eventBits = xEventGroupClearBits(xEventGroup, BIT_0);
    }
  }
}
void ObjectDetect(void *parameter)
{
  while (1)
  {
    unsigned long duration; // biến đo thời gian
    int distance;           // biến lưu khoảng cách
    digitalWrite(trig, 0);  // tắt chân trig
    delayMicroseconds(2);
    digitalWrite(trig, 1); // phát xung từ chân trig
    delayMicroseconds(5);
    digitalWrite(trig, 0); // tắt chân trig
    duration = pulseIn(echo, HIGH);
    distance = int(duration / 2 / 29.412);
    Serial.print(distance);
    Serial.println("cm");
    if (distance < 10)
    {
      eventBits = xEventGroupSetBits(xEventGroup, BIT_1);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
void warningControl(void *parameter)
{
  while (1)
  {
    eventBits = xEventGroupGetBits(xEventGroup);
    if ((eventBits & (BIT_2)) != 0)
    {
      digitalWrite(13, HIGH);
      eventBits = xEventGroupClearBits(xEventGroup, BIT_2);
    }
    if ((eventBits & (BIT_3)) != 0)
    {
      digitalWrite(13, LOW);
      eventBits = xEventGroupClearBits(xEventGroup, BIT_3);
    }
  }
}
void waterControl(void *parameter)
{
  while (1)
  {
    bool var;
    eventBits = xEventGroupGetBits(xEventGroup);
    if ((eventBits & (BIT_1)) != 0)
    {
      xQueuePeek(xQueueVar, &(var), (TickType_t)10);
      Serial.println(0);
      Serial.println("ISR");
      for (int pos = 0; pos <= 180; pos += 30)
      {                     // goes from 0 degrees to 180 degrees
        myservo.write(pos); // tell servo to go to position in variable 'pos'
        vTaskDelay(20 / portTICK_PERIOD_MS);
      }
      for (int pos = 180; pos >= 0; pos -= 30)
      {                     // goes from 180 degrees to 0 degrees
        myservo.write(pos); // tell servo to go to position in variable 'pos'
        vTaskDelay(20 / portTICK_PERIOD_MS);
      }
      Serial.println(1);
      eventBits = xEventGroupClearBits(xEventGroup, BIT_1); /* The bits being cleared. */
    }
  }
}
void setup()
{
  Serial.begin(9600);
  pinMode(trig, OUTPUT); // chân trig sẽ phát tín hiệu
  pinMode(echo, INPUT);  // chân echo sẽ nhận tín hiệu
  pinMode(13, OUTPUT);   // led
  pinMode(5, OUTPUT);    // servo
  pinMode(12, INPUT);
  digitalWrite(13, LOW); // led
  myservo.attach(5);
  mlx.begin();
  delay(100);
  lcd.init();
  lcd.backlight();
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  xQueueAmbient = xQueueCreate(1, sizeof(int));
  xQueueObject = xQueueCreate(1, sizeof(int));
  xQueueVar = xQueueCreate(1, sizeof(bool));
  xEventGroup = xEventGroupCreate();
  xSemaphore = xSemaphoreCreateBinary();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
      ObjectDetect,        // Function to be called
      "ObjectDetect",      // Name of task
      6000,                // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu1);           // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
      waterControl,        // Function to be called
      "waterControl",      // Name of task
      3000,                // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu1);           // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
      warningControl,      // Function to be called
      "warningControl",    // Name of task
      2000,                // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu1);           // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS	printValue,  // Function to be called
      "printValue",        // Name of task
      2048,                // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu0);           // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
      getValue,            // Function to be called
      "getValue",          // Name of task
      18000,               // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu0);           // Run on one core for demo purposes (ESP32 only)
  xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
      lcdControl,          // Function to be called
      "lcdControl",        // Name of task
      3000,                // Stack size (bytes in ESP32, words in FreeRTOS)
      NULL,                // Parameter to pass to function
      1,                   // Task priority (0 to configMAX_PRIORITIES - 1)
      NULL,                // Task handle
      app_cpu0);           // Run on one core for demo purposes (ESP32 only)
}
void loop()
{
}
