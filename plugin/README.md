# auma-plugin

JUCE plugin (AU + VST3) for the AudIA Mastering Assistant.

The plugin is a transparent tap on each track. It measures the cheap
real-time metrics that drive its UI and the standalone's telemetry,
and captures audio on demand for the backend's deep analysis. The
canonical analysis (psychoacoustics, full integrated LUFS, MIR,
spectrograms) runs on the Python sidecar; the plugin does not
duplicate it.

## Real-time invariants

`AudioProcessor::processBlock` runs on the realtime audio thread. It
must never allocate, lock, do I/O or network. The plugin enforces
this by copying samples into a `juce::AbstractFifo` and doing all
metric work, serialisation and WebSocket sends on a worker thread
(`juce::Timer`).

## Build

```bash
cd plugin
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target auma_plugin_AU auma_plugin_VST3 -j
```

On macOS the AU lands in `~/Library/Audio/Plug-Ins/Components/AuMA.component`
and the VST3 in `~/Library/Audio/Plug-Ins/VST3/AuMA.vst3` thanks to
`COPY_PLUGIN_AFTER_BUILD`. Restart Logic Pro to pick up the AU after
the first install.

JUCE is fetched via CMake FetchContent at configure time; the first
configure downloads roughly 100 MB.
