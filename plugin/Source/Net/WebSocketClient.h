#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include <juce_core/juce_core.h>

namespace ix {
class WebSocket;
}

namespace auma::net {

struct RegisterPayload;
struct MetricsPayload;

/**
 * WebSocket client that lives in its own background thread (ixwebsocket
 * manages the IO thread). The plugin's audio thread NEVER touches this
 * object; only the worker thread (the LufsMeter timer) and the
 * processor's prepare/release calls do.
 *
 * The client auto-reconnects with exponential backoff (configured in
 * ixwebsocket). On every successful connect it re-sends the register
 * frame so the standalone keeps the instance in its registry.
 */
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    void setRegister(RegisterPayload payload);
    void start(const juce::String& url);
    void stop();

    /** Worker-thread call to push the latest telemetry frame. Drops if
     *  the socket is not currently open. */
    void sendMetrics(const MetricsPayload& payload);

    [[nodiscard]] bool isConnected() const noexcept {
        return connected_.load(std::memory_order_relaxed);
    }

private:
    void onOpen();
    void resendRegister();

    std::unique_ptr<ix::WebSocket> ws_;
    std::mutex registerMutex_;
    std::unique_ptr<RegisterPayload> registerPayload_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> started_{false};
};

}  // namespace auma::net
