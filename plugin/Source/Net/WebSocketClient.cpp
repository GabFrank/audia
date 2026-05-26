#include "WebSocketClient.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketMessage.h>

#include "Protocol.h"

namespace auma::net {

namespace {

struct NetSystemGuard {
    NetSystemGuard() { ix::initNetSystem(); }
    ~NetSystemGuard() { ix::uninitNetSystem(); }
};

// Initialise the platform networking stack exactly once for the process.
void ensureNetSystem() {
    static NetSystemGuard guard;
    (void)guard;
}

}  // namespace

WebSocketClient::WebSocketClient() {
    ensureNetSystem();
    ws_ = std::make_unique<ix::WebSocket>();
}

WebSocketClient::~WebSocketClient() {
    stop();
}

void WebSocketClient::setRegister(RegisterPayload payload) {
    std::lock_guard lock(registerMutex_);
    registerPayload_ = std::make_unique<RegisterPayload>(std::move(payload));
    if (connected_.load(std::memory_order_relaxed)) {
        resendRegister();
    }
}

void WebSocketClient::start(const juce::String& url) {
    bool expected = false;
    if (!started_.compare_exchange_strong(expected, true)) {
        return;
    }
    ws_->setUrl(url.toStdString());
    ws_->setHandshakeTimeout(2);
    ws_->disablePerMessageDeflate();
    ws_->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
            case ix::WebSocketMessageType::Open:
                connected_.store(true, std::memory_order_relaxed);
                onOpen();
                break;
            case ix::WebSocketMessageType::Close:
            case ix::WebSocketMessageType::Error:
                connected_.store(false, std::memory_order_relaxed);
                break;
            default:
                break;
        }
    });
    ws_->start();
}

void WebSocketClient::stop() {
    if (!started_.exchange(false)) return;
    if (ws_) ws_->stop();
    connected_.store(false, std::memory_order_relaxed);
}

void WebSocketClient::onOpen() {
    resendRegister();
}

void WebSocketClient::resendRegister() {
    std::lock_guard lock(registerMutex_);
    if (!registerPayload_) return;
    const auto json = buildRegisterJson(*registerPayload_);
    ws_->send(json.toStdString());
}

void WebSocketClient::sendMetrics(const MetricsPayload& payload) {
    if (!connected_.load(std::memory_order_relaxed)) return;
    const auto json = buildMetricsJson(payload);
    ws_->send(json.toStdString());
}

}  // namespace auma::net
