#pragma once

#include <juce_core/juce_core.h>

namespace auma::net {

struct RegisterPayload {
    juce::String instanceId;
    juce::String format;        // "AU" | "VST3" | "Standalone"
    juce::String role;          // "track" | "bus" | "master"
    juce::String trackName;     // may be empty
    double sampleRate = 0.0;
    int channels = 0;
    bool isFirstInChain = false;
    juce::String pluginVersion;
};

struct MetricsPayload {
    juce::String instanceId;
    double timestampMillis = 0.0;
    bool hasLufsMomentary = false;
    double lufsMomentary = 0.0;
};

[[nodiscard]] juce::String buildRegisterJson(const RegisterPayload& p);
[[nodiscard]] juce::String buildMetricsJson(const MetricsPayload& p);

/** ISO-style UUID v4 string in JUCE form. */
[[nodiscard]] juce::String makeInstanceId();

}  // namespace auma::net
