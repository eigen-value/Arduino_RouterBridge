#pragma once

#ifndef BRIDGE_IMOLA_H
#define BRIDGE_IMOLA_H

#define RESET_METHOD "$/reset"
#define BIND_METHOD "$/register"

#include <Arduino_RPClite.h>


class Bridge: public RPCClient, public RPCServer {

    SemaphoreHandle_t write_mutex = NULL;

    SemaphoreHandle_t read_mutex = NULL;

public:

    Bridge(ITransport& t) : RPCClient(t), RPCServer(t) {}

    Bridge(Stream& stream) : RPCClient(stream), RPCServer(stream) {}

    // Initialize the bridge
    bool begin() {
        bool res;
        write_mutex = xSemaphoreCreateMutex();
        read_mutex = xSemaphoreCreateMutex();
        return call(RESET_METHOD, res);
    }

    template<typename F>
    bool provide(const MsgPack::str_t& name, F&& func) {
        bool res;
        Serial.print("providing method: ");
        Serial.println(name);
        if (!call(BIND_METHOD, res, name)) {
            Serial.println("-error");
            return false;
        }
        Serial.println("-done");
        return bind(name, func);
    }

    void update() {
        while (true){
            // Acquire Read LOCK
            if (xSemaphoreTake( read_mutex, ( TickType_t ) 10 ) == pdTRUE){
                if (get_rpc()) {
                    // Release Read LOCK
                    xSemaphoreGive(read_mutex);
                    break;
                }
                xSemaphoreGive(read_mutex); // Release if no RPC
                vTaskDelay(1); // Yield to other tasks (optional but good practice)
            } else {
                vTaskDelay(1);
            }
        }

        process_request();
        
        while (true){
            // Acquire Write LOCK
            if (xSemaphoreTake( write_mutex, ( TickType_t ) 10 ) != pdTRUE){
                send_response();
                xSemaphoreGive(write_mutex);
                vTaskDelay(1);
                break;
                // Release Write LOCK
            } else {
                vTaskDelay(1);
            }
        }

    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {
        // Protect the following calls with a mutex if necessary

        while (true) {
            // Acquire Write LOCK
            if (xSemaphoreTake( write_mutex, ( TickType_t ) 10 ) != pdTRUE){
                send_rpc(method, std::forward<Args>(args)...);
                xSemaphoreGive(write_mutex);
                vTaskDelay(1);
                break;
                // Release Write LOCK
            } else {
                vTaskDelay(1);
            }
        }


        while (true) {
            if (xSemaphoreTake(read_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                // Critical section starts
                if (get_response(result)) {
                    xSemaphoreGive(read_mutex); // Release before breaking
                    vTaskDelay(1);
                    break;
                }
                xSemaphoreGive(read_mutex); // Release if no response
                vTaskDelay(1); // Yield to other tasks (optional but good practice)
            } else {
                // Optional: Log a warning if semaphore is stuck
                // ESP_LOGW("TAG", "Failed to take read_mutex");
                vTaskDelay(1); // Prevent tight loop
            }
        }

        return (lastError.code == NO_ERR);
    }

    String get_error_message() const {
        return (String) lastError.traceback;
    }

    uint8_t get_error_code() const {
        return (uint8_t) lastError.code;
    }

};

#endif // BRIDGE_IMOLA_H