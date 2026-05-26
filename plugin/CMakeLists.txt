cmake_minimum_required(VERSION 3.22)

project(auma_plugin VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS deployment target. Logic Pro on Apple silicon ships with newer
# SDKs but 11.0 covers everything reasonable in the field.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

include(FetchContent)
# JUCE 7 is pinned because JUCE 8 requires Xcode 15.1 or newer, and the
# current dev box ships Xcode 15.0. When the toolchain is bumped we can
# move forward.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(juce)

# ---------------------------------------------------------------------------
# AuMA audio plugin (AU + VST3).
#
# Manufacturer code AuMA, plugin code Mast. These are provisional and will
# be revisited before public release.
# ---------------------------------------------------------------------------
juce_add_plugin(auma_plugin
    PRODUCT_NAME              "AuMA"
    COMPANY_NAME              "AuMA project"
    BUNDLE_ID                 "org.auma.plugin"
    PLUGIN_MANUFACTURER_CODE  AuMA
    PLUGIN_CODE               Mast
    FORMATS                   AU VST3
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   TRUE
    AU_MAIN_TYPE              kAudioUnitType_Effect
    VST3_CATEGORIES           Analyzer Tools
)

target_sources(auma_plugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(auma_plugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(auma_plugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
