#include <Arduino_RouterBridge.h>

SerialTransport ser(Serial0);
Bridge bridge(ser);

String greet() {
    return String("Hello Friend");
}

int lengthyOp() {
  Serial.println("lengthy operation...");
  unsigned long startTime = millis();
  int result = 0;

  while (millis() - startTime < 3000) {
    result = (result + 1) % 1000 ;
  }
  Serial.print("-lengthyOp done returning: ");
  Serial.println(result);
  return result;
}

void serve(void * pvParameters) {
    while(1){
        Serial.println("...upd");
        bridge.update();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void silly_call(void * pvParameters) {
    int rand_int;
    while(1){
        Serial.println("...call");
        bool ok = bridge.call("get_rand", rand_int);
        if (ok) {
            Serial.print("got rand_int: ");
            Serial.println(rand_int);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Wait 1 second
    }

}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while (!Serial);

    Serial0.begin(115200);
    while (!Serial0);

    if (!bridge.begin()) {
        Serial.println("Error initializing bridge");
        while(1);
    } else {
        Serial.println("Bridge initialized successfully");
    }

    bridge.provide("greet", greet);

    bridge.provide("lengthyOp", lengthyOp);

    xTaskCreate(&serve, "serve", 10000, NULL, 0, NULL);
    xTaskCreate(&silly_call, "silly_call", 10000, NULL, 0, NULL);

}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100);
}