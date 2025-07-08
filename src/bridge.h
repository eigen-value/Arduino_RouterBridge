#pragma once

#ifndef BRIDGE_IMOLA_H
#define BRIDGE_IMOLA_H

#define RESET_METHOD "$/reset"
#define BIND_METHOD "$/register"

#include <Arduino_RPClite.h>


class Bridge: public RPCClient, public RPCServer {

    SemaphoreHandle_t write_mutex;

    SemaphoreHandle_t read_mutex;

public:

    Bridge(ITransport& t) : RPCClient(t), RPCServer(t) {
        write_mutex = xSemaphoreCreateMutex();
        read_mutex = xSemaphoreCreateMutex();
    }

    Bridge(Stream& stream) : RPCClient(stream), RPCServer(stream) {
        write_mutex = xSemaphoreCreateMutex();
        read_mutex = xSemaphoreCreateMutex();
    }

    ~Bridge() {
        vSemaphoreDelete(write_mutex);
        vSemaphoreDelete(read_mutex);
    }

    // Initialize the bridge
    bool begin() {
        bool res;
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
            if (xSemaphoreTake( read_mutex, 10 )){
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
            if (xSemaphoreTake( write_mutex, 10 )){
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
            if (xSemaphoreTake( write_mutex, 10 )){
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
            if (xSemaphoreTake(read_mutex, 10 )) {
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