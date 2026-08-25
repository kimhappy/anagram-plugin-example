#pragma once

#define DISTRHO_PLUGIN_NAME             "Anagram Example"
#define DISTRHO_PLUGIN_URI              "https://github.com/kimhappy/anagram-plugin-example"
#define DISTRHO_PLUGIN_MAINTAINER_EMAIL "babtul21@gmail.com"
#define DISTRHO_PLUGIN_MAKER            "kimhappy"
#define DISTRHO_PLUGIN_LICENSE          "ISC"
#define DISTRHO_PLUGIN_DESCRIPTION      "Stereo 6 band equalizer."
#define DISTRHO_PLUGIN_SHORTDESC        DISTRHO_PLUGIN_DESCRIPTION
#define DISTRHO_PLUGIN_HOMEPAGE         DISTRHO_PLUGIN_URI
#define DISTRHO_PLUGIN_LV2_PROJECT      DISTRHO_PLUGIN_URI

// Two or three uppercase characters
#define DISTRHO_PLUGIN_ABBREVIATION "SEQ"

// 200x200 PNGs.
#define DISTRHO_PLUGIN_ANAGRAM_BLOCK_IMAGE_OFF "off.png"
#define DISTRHO_PLUGIN_ANAGRAM_BLOCK_IMAGE_ON  "on.png"

// The symbol of the parameter the pot under the block on the signal chain screen adjusts.
#define DISTRHO_PLUGIN_ANAGRAM_QUICK_POT "left_62hz"

// Four characters identifying the plugin.
#define DISTRHO_PLUGIN_UNIQUE_ID AgEq

// The device build defines _DARKGLASS_DEVICE_PABLITO.
// DPF asks the host for a license file and silences an unlicensed instance's output.
#ifdef _DARKGLASS_DEVICE_PABLITO
#    define DISTRHO_PLUGIN_LICENSED_FOR_MOD 1
#endif

// Anagram only supports 1in-1out or 2in-2out.
#define DISTRHO_PLUGIN_NUM_INPUTS  2
#define DISTRHO_PLUGIN_NUM_OUTPUTS DISTRHO_PLUGIN_NUM_INPUTS

// https://github.com/Darkglass-Electronics/Plugin-Dev-Setup/blob/main/CATEGORIES.md
#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:MultiEQPlugin"

// Anagram default configuration.
#define DISTRHO_PLUGIN_HAS_UI                              0
#define DISTRHO_PLUGIN_IS_RT_SAFE                          1
#define DISTRHO_PLUGIN_IS_SYNTH                            0
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS                  0
#define DISTRHO_PLUGIN_WANT_FULL_STATE                     0
#define DISTRHO_PLUGIN_WANT_LATENCY                        0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT                     0
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT                    0
#define DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST 0
#define DISTRHO_PLUGIN_WANT_PROGRAMS                       0
#define DISTRHO_PLUGIN_WANT_STATE                          0
#define DISTRHO_PLUGIN_WANT_TIMEPOS                        0
