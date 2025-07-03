#pragma once

#ifndef BRIDGE_IMOLA_H
#define BRIDGE_IMOLA_H

#define RESET_METHOD "$/reset"
#define BIND_METHOD "$/register"

#include <Arduino_RPClite.h>


class Bridge: public RPCClient, public RPCServer {

public:

    Bridge(ITransport& t) : RPCClient(t), RPCServer(t) {}

    Bridge(Stream& stream) : RPCClient(stream), RPCServer(stream) {}

    // Initialize the bridge
    bool begin() {
        bool res;
        return call(RESET_METHOD, res);
    }

    template<typename F>
    bool provide(const MsgPack::str_t& name, F&& func) {
        bool res;
        if (!call(BIND_METHOD, res, name)) {
            return false;
        }
        return bind(name, func);
    }

    void update() {
        // Read LOCK
        get_rpc();
        // END Read LOCK

        process_request();
        
        // Write LOCK
        send_response();
        // END Write LOCK
    }

    template<typename RType, typename... Args>
    bool call(const MsgPack::str_t method, RType& result, Args&&... args) {
        // Protect the following calls with a mutex if necessary

        // Write LOCK
        if(!send_rpc(method, std::forward<Args>(args)...)) {
            lastError.code = GENERIC_ERR;
            lastError.traceback = "Failed to send RPC call";
            return false;
        }
        // END Write LOCK

        while (true) {
            // Read LOCK
            if (get_response(result)) {
                break;
            }
            // END Read LOCK
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