#pragma once

// Registers the self.coglet.* MCP tools that agent-hub discovers over the
// voice WebSocket. The controller itself lives in github.com/ricklon/coglet
// (firmware/coglet_controller) and knows nothing about xiaozhi's MCP server.
void RegisterCogletMcpTools();
