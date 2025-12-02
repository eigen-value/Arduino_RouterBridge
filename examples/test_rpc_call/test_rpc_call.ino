#include <Arduino_RouterBridge.h>
#include <zephyr/kernel.h>

void setup() {
    Serial.begin(115200);
    k_sleep(K_MSEC(2000));
    Serial.println("\n=== Simple RPC Call Test ===\n");

    Bridge.begin();
    Monitor.begin();

    // Create RPC call
    RpcCall loopback_call = Bridge.call("loopback", "TEST");

    Serial.println("--- Initial State (Before Execution) ---");
    Serial.println("Call error state before execution.");
    Serial.println("Expecting GENERIC_ERR (0xFF) with message 'This call is not yet executed'");
    Serial.print("Error Code: 0x");
    Serial.println(loopback_call.getErrorCode(), HEX);
    Serial.print("Error Message: ");
    Serial.println(loopback_call.getErrorMessage().c_str());
    Serial.print("Is Error: ");
    Serial.println(loopback_call.isError() ? "YES" : "NO");

    Serial.println("--- First result() call ---");
    MsgPack::str_t message1;
    bool success1 = loopback_call.result(message1);

    if (success1) {
        Serial.print("Success: ");
        Serial.println(message1.c_str());
    } else {
        Serial.print("Failed: 0x");
        Serial.println(loopback_call.getErrorCode(), HEX);
    }

    Serial.println("\n--- Second result() call (same object) ---");
    MsgPack::str_t message2;
    bool success2 = loopback_call.result(message2);

    if (success2) {
        Serial.print("Success: ");
        Serial.println(message2.c_str());
    } else {
        Serial.print("Failed: 0x");
        Serial.println(loopback_call.getErrorCode(), HEX);
        Serial.print("Error: ");
        Serial.println(loopback_call.getErrorMessage().c_str());
    }
}

void loop() {
    k_sleep(K_MSEC(5000));
}