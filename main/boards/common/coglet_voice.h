#pragma once

// The voice loop both Coglet boards share: wake word (or always listening
// without one), quiet while in calibration mode, and the session, which
// carries MCP, kept open in every mode.
void StartCogletVoiceLoop();
