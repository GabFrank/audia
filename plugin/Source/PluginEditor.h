#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace auma {

class AumaProcessor;

class AumaEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AumaEditor(AumaProcessor& p);
    ~AumaEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    AumaProcessor& processor_;
    juce::Label lufsLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AumaEditor)
};

}  // namespace auma
