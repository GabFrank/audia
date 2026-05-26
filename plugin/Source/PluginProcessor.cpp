#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace auma {

namespace {

constexpr const char* kBackendUrl = "ws://127.0.0.1:17600/ws/plugin";
constexpr int kWorkerHz = 30;
constexpr int kMetricsHz = 3;
constexpr int kStateXmlTag = 0;  // placeholder for future use

juce::String detectFormat() {
    switch (juce::PluginHostType::getPluginLoadedAs()) {
        case juce::AudioProcessor::wrapperType_AudioUnit:
        case juce::AudioProcessor::wrapperType_AudioUnitv3:
            return "AU";
        case juce::AudioProcessor::wrapperType_VST3:
            return "VST3";
        case juce::AudioProcessor::wrapperType_Standalone:
            return "Standalone";
        default:
            return "Standalone";
    }
}

}  // namespace

AumaProcessor::AumaProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      instanceId_(net::makeInstanceId()) {}

AumaProcessor::~AumaProcessor() {
    workerTimer_.stopTimer();
    wsClient_.stop();
}

void AumaProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    currentSampleRate_ = sampleRate;
    currentChannels_ = getTotalNumInputChannels();

    lufsMeter_.prepare(sampleRate, currentChannels_);
    rebuildRegisterPayload();
    wsClient_.start(kBackendUrl);
    workerTimer_.startTimerHz(kWorkerHz);
}

void AumaProcessor::releaseResources() {
    workerTimer_.stopTimer();
    lufsMeter_.reset();
}

bool AumaProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == mainOut;
}

void AumaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Realtime-safe: lock-free push into the FIFO and nothing else.
    // The worker thread drains the FIFO, runs metrics, and dispatches
    // the WebSocket frame.
    lufsMeter_.push(buffer.getArrayOfReadPointers(),
                    buffer.getNumChannels(),
                    buffer.getNumSamples());
}

juce::AudioProcessorEditor* AumaProcessor::createEditor() {
    return new AumaEditor(*this);
}

void AumaProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ValueTree state("AumaState");
    state.setProperty("instanceId", instanceId_, nullptr);
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
    juce::ignoreUnused(kStateXmlTag);
}

void AumaProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    auto state = juce::ValueTree::readFromStream(stream);
    if (!state.isValid()) return;
    const auto stored = state.getProperty("instanceId").toString();
    if (stored.isNotEmpty()) {
        instanceId_ = stored;
        rebuildRegisterPayload();
    }
}

void AumaProcessor::rebuildRegisterPayload() {
    net::RegisterPayload payload;
    payload.instanceId = instanceId_;
    payload.format = detectFormat();
    payload.role = "track";  // user-editable later; default per the spec.
    payload.trackName = {};
    payload.sampleRate = currentSampleRate_ > 0.0 ? currentSampleRate_ : 48000.0;
    payload.channels = currentChannels_ > 0 ? currentChannels_ : 2;
    payload.isFirstInChain = false;
    payload.pluginVersion = JucePlugin_VersionString;
    wsClient_.setRegister(std::move(payload));
}

void AumaProcessor::onWorkerTick() {
    lufsMeter_.update();

    // Throttle telemetry dispatch to kMetricsHz.
    constexpr int kDivisor = kWorkerHz / kMetricsHz;
    if (++metricsCounter_ < kDivisor) return;
    metricsCounter_ = 0;

    if (!wsClient_.isConnected()) return;

    net::MetricsPayload payload;
    payload.instanceId = instanceId_;
    payload.timestampMillis =
        static_cast<double>(juce::Time::currentTimeMillis());
    const float lufs = lufsMeter_.currentLufs();
    if (lufs > metrics::LufsMeter::kSilenceFloorLufs + 0.1f) {
        payload.hasLufsMomentary = true;
        payload.lufsMomentary = static_cast<double>(lufs);
    }
    wsClient_.sendMetrics(payload);
}

}  // namespace auma

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new auma::AumaProcessor();
}
