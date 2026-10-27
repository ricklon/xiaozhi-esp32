#pragma once

// The voice loop both Coglet boards share: always listening, quiet while in
// calibration mode, and the session (which carries MCP) kept open while quiet.
void StartCogletVoiceLoop();
