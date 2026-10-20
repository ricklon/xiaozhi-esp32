#include "coglet_mcp_tools.h"
#include "coglet_controller.h"
#include "mcp_server.h"

#include <string>

// Conversation tools only. Engagement, mapping, calibration and raw servo
// commands stay local (serial or the builder page), so the LLM can stop the
// robot but can never re-energise it.
void RegisterCogletMcpTools() {
    auto* c = &CogletController::Instance();
    auto& m = McpServer::GetInstance();
    m.AddTool("self.coglet.gaze","Bounded base/tilt gaze (-100..100 within calibrated endpoints), with upper-lid tracking. Requires local engagement.",
        PropertyList({Property("horizontal",kPropertyTypeInteger,0,-100,100),Property("vertical",kPropertyTypeInteger,0,-100,100)}),
        [c](const PropertyList& p)->ReturnValue {return c->Command("gaze "+std::to_string(p["horizontal"].value<int>())+" "+std::to_string(p["vertical"].value<int>()));});
    m.AddTool("self.coglet.blink","Start one device-timed blink; requires established gaze and local engagement.",PropertyList(),
        [c](const PropertyList&)->ReturnValue{return c->Command("blink");});
    m.AddTool("self.coglet.animate","Start a device-timed expression: look, roll, side_eye, wink, surprise, sleepy. Poll state for completion or later faults.",
        PropertyList({Property("name",kPropertyTypeString)}),
        [c](const PropertyList& p)->ReturnValue{return c->Command("animate "+p["name"].value<std::string>());});
    m.AddTool("self.coglet.stop","Stop animation at the last commanded pose. Automatic blinking resumes after a settling interval; use release to prevent all motion.",PropertyList(),
        [c](const PropertyList&)->ReturnValue{return c->Command("stop");});
    m.AddTool("self.coglet.release","Latch all servo outputs off. Only local builder controls can engage again. This does not cut servo power.",PropertyList(),
        [c](const PropertyList&)->ReturnValue{return c->Command("release");});
    m.AddTool("self.coglet.state","Diagnostic state, calibration and commanded positions (no position feedback), release latch and hardware errors.",PropertyList(),
        [c](const PropertyList&)->ReturnValue{return c->Command("state");});
}
