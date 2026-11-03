cmake_minimum_required(VERSION 3.22)

# Runtime C++ de MSVC lié en statique (/MT) : le plugin n'a pas besoin du
# "Visual C++ Redistributable" sur le PC de destination.
# Doit être défini avant project() pour s'appliquer à toutes les cibles, JUCE compris.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(WaveSketch VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "WaveSketch se compile uniquement en 64 bits (utilise -A x64).")
endif()

# --- JUCE ---------------------------------------------------------------------
# Récupéré automatiquement à la configuration, figé sur une version précise.
# Pour éviter le téléchargement, on peut pointer vers une copie locale avec
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=C:/chemin/vers/JUCE
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(JUCE)

# --- Plugin -------------------------------------------------------------------
juce_add_plugin(WaveSketch
    COMPANY_NAME                "WaveSketch Audio"
    PLUGIN_MANUFACTURER_CODE    Wskt
    PLUGIN_CODE                 Wvsk
    PRODUCT_NAME                "WaveSketch"
    BUNDLE_ID                   com.wavesketchaudio.wavesketch
    DESCRIPTION                 "Wavetable synth with a hand-drawn waveform"
    FORMATS                     VST3 Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             "Instrument" "Synth"
    # Pas de copie automatique dans C:\Program Files\Common Files\VST3 :
    # ce dossier est protégé et exigerait de compiler en administrateur.
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(WaveSketch
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

target_include_directories(WaveSketch PRIVATE Source)

target_compile_definitions(WaveSketch
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(WaveSketch
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
