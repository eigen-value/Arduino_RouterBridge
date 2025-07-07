#include <Arduino_BridgeImola.h>

Bridge bridge(Serial0);

String greet() {
    return String("Hello Friend");
}

// int lengthyOp() {
//   Serial.println("lengthy operation...");
//   unsigned long startTime = millis();
//   int result = 0;

//   while (millis() - startTime < 3000) {
//     result = (result + 1) % 1000 ;
//   }
//   Serial.print("-lengthyOp done returning: ");
//   Serial.println(result);
//   return result;
// }

int lengthyOp(){
    return 88;
}

void serve(void * pvParameters) {
    bridge.update();
    delay(1);
}

void silly_call(void * pvParameters) {
    int rand_int;
    bool ok = bridge.call("get_rand", rand_int);

    if (ok) {
        Serial.print("Random int from server: ");
        Serial.println(rand_int);
    }

    delay(100);
}

bool set_led(bool state) {
    digitalWrite(LED_BUILTIN, state);
    return state;
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
    //xTaskCreate(&silly_call, "silly_call", 10000, NULL, 0, NULL);

}

void loop() {
    bool val = false;

    set_led(val);
    val = !val;

    delay(500);

}