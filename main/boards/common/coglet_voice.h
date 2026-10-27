#pragma once

#include <string>

// The voice loop both Coglet boards share: wake word (or always listening
// without one), quiet while in calibration mode, and the session, which
// carries MCP, kept open in every mode.
void StartCogletVoiceLoop();

// The wake phrase setting behind !wake and self.voice.wake_word:
//   ""/"status"        what the robot listens for
//   "default"          back to the build's phrase and threshold
//   "threshold N"      1-99; lower is more sensitive
//   anything else      the new phrase (English words)
// Returns what to tell the person. *changed means a new setting was saved; it
// takes effect at the next boot, so the caller reboots. Throws on bad input or
// on a build whose wake word can't be changed.
std::string CogletWakeWord(const std::string& args, bool* changed);
