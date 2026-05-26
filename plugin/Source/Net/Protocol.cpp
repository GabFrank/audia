#include "Protocol.h"

namespace auma::net {

juce::String buildRegisterJson(const RegisterPayload& p) {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "register");
    obj->setProperty("protocolVersion", "0.1");
    obj->setProperty("instanceId", p.instanceId);
    obj->setProperty("format", p.format);
    obj->setProperty("role", p.role);
    if (p.trackName.isNotEmpty()) obj->setProperty("trackName", p.trackName);
    obj->setProperty("sampleRate", p.sampleRate);
    obj->setProperty("channels", p.channels);
    obj->setProperty("isFirstInChain", p.isFirstInChain);
    if (p.pluginVersion.isNotEmpty()) obj->setProperty("pluginVersion", p.pluginVersion);
    return juce::JSON::toString(juce::var(obj.get()), true);
}

juce::String buildMetricsJson(const MetricsPayload& p) {
    juce::DynamicObject::Ptr metrics = new juce::DynamicObject();
    if (p.hasLufsMomentary) metrics->setProperty("lufsMomentary", p.lufsMomentary);

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "metrics");
    obj->setProperty("instanceId", p.instanceId);
    obj->setProperty("timestamp", p.timestampMillis);
    obj->setProperty("metrics", juce::var(metrics.get()));
    return juce::JSON::toString(juce::var(obj.get()), true);
}

juce::String makeInstanceId() {
    return juce::Uuid().toDashedString();
}

}  // namespace auma::net
