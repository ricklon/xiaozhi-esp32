// Motion keyframes and upper-lid coupling adapted from eyemech-esp32-xiao
// components/eye_motion/eye_motion.c. See README.md for deliberate changes.
#include "coglet_controller.h"
#include "config.h"
#include "mcp_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdio>

namespace {
constexpr const char* names[] = {"base", "tilt", "lid_left", "lid_right", "mouth", "ears"};
struct Frame { float lr, ud, lid_l, lid_r; int ms; };
static const Frame s_frames_look[] = {
    { 0.50f, 0.50f, 1.00f, 1.00f, 400 },   /* open, centred             */
    { 0.00f, NAN,   NAN,   NAN,   700 },   /* look left                 */
    { 1.00f, NAN,   NAN,   NAN,  1100 },   /* sweep across to the right */
    { 0.50f, NAN,   NAN,   NAN,   600 },   /* back to centre            */
    { NAN,   NAN,   0.00f, 0.00f, 400 },   /* close                     */
};

/* A circle in gaze space. Lids left to the coupling throughout, so the eyes
 * hood through the bottom and widen over the top — that is most of what makes
 * it read as a roll rather than a mechanical sweep. */
static const Frame s_frames_roll[] = {
    { 0.50f, 0.50f, 1.00f, 1.00f, 500 },   /* open, centred   */
    { 0.50f, 0.92f, NAN,   NAN,   500 },   /* up              */
    { 0.80f, 0.80f, NAN,   NAN,   350 },
    { 0.92f, 0.50f, NAN,   NAN,   350 },   /* right           */
    { 0.80f, 0.20f, NAN,   NAN,   350 },
    { 0.50f, 0.08f, NAN,   NAN,   350 },   /* down            */
    { 0.20f, 0.20f, NAN,   NAN,   350 },
    { 0.08f, 0.50f, NAN,   NAN,   350 },   /* left            */
    { 0.20f, 0.80f, NAN,   NAN,   350 },
    { 0.50f, 0.92f, NAN,   NAN,   350 },   /* back to the top */
    { 0.50f, 0.50f, NAN,   NAN,   600 },   /* settle centred  */
};

/* Suspicion. The snap across is quick, the lids narrow to a slit, and then it
 * HOLDS — the hold is the whole emote. Coming back is slower than going. */
static const Frame s_frames_side_eye[] = {
    { 0.50f, 0.50f, 0.90f, 0.90f, 300 },
    { 0.12f, 0.56f, 0.45f, 0.45f, 350 },   /* dart across, lids narrow */
    { 0.12f, 0.56f, 0.45f, 0.45f,1300 },   /* hold the look            */
    { 0.50f, 0.50f, 0.90f, 0.90f, 550 },   /* unhurried return         */
};

/* Only possible because the lids are per-eye while the gaze is shared. */
static const Frame s_frames_wink[] = {
    { 0.50f, 0.50f, 0.95f, 0.95f, 300 },   /* both open        */
    { NAN,   NAN,   0.00f, 0.95f, 170 },   /* left shuts, fast */
    { NAN,   NAN,   0.00f, 0.95f, 200 },   /* held shut        */
    { NAN,   NAN,   0.95f, 0.95f, 260 },   /* and back         */
};

/* Fast attack, long hold, slow release — the shape of a startle. */
static const Frame s_frames_surprise[] = {
    { 0.50f, 0.50f, 0.55f, 0.55f, 250 },   /* half-lidded first, for contrast */
    { 0.50f, 0.70f, 1.00f, 1.00f, 110 },   /* snap wide, gaze lifts           */
    { 0.50f, 0.70f, 1.00f, 1.00f, 950 },   /* hold                            */
    { 0.50f, 0.50f, 0.85f, 0.85f, 800 },   /* settle back down                */
};

/* Everything slow. Heaviness is pace, not position. */
static const Frame s_frames_sleepy[] = {
    { 0.50f, 0.50f, 0.85f, 0.85f, 600 },
    { 0.47f, 0.35f, 0.35f, 0.35f,1500 },   /* lids droop, gaze sinks */
    { 0.46f, 0.30f, 0.00f, 0.00f, 900 },   /* slow close             */
    { 0.46f, 0.30f, 0.00f, 0.00f, 800 },   /* stays shut a beat      */
    { 0.50f, 0.44f, 0.55f, 0.55f,1200 },   /* half open, still heavy */
};


struct Animation { const char* name; const Frame* frames; int count; };
#define ANIM(n) {#n, s_frames_##n, int(sizeof(s_frames_##n)/sizeof(Frame))}
const Animation animations[] = {ANIM(look), ANIM(roll), ANIM(side_eye), ANIM(wink), ANIM(surprise), ANIM(sleepy)};
#undef ANIM
int64_t Now() { return esp_timer_get_time()/1000; }
int64_t BlinkTime(int64_t now) { return now + 2000 + esp_random()%5001; }
int AxisIndex(const std::string& name) {
    for (int i=0; i<6; ++i) if (name == names[i]) return i;
    return -1;
}
bool Unit(float v) { return std::isfinite(v) && v >= 0 && v <= 1; }
std::string Json(cJSON* root) {
    char* text = cJSON_PrintUnformatted(root);
    std::string result = text ? text : "{}";
    cJSON_free(text); cJSON_Delete(root); return result;
}
}

CogletController& CogletController::Instance() { static CogletController c; return c; }

esp_err_t CogletController::Register(uint8_t reg, uint8_t value) {
    if (!device_) return ESP_ERR_INVALID_STATE;
    uint8_t bytes[] = {reg, value};
    return i2c_master_transmit(device_, bytes, sizeof(bytes), 50);
}
esp_err_t CogletController::Probe() {
    if (!ready_) return last_error_ == ESP_OK ? ESP_ERR_INVALID_STATE : last_error_;
    uint8_t reg=0, mode=0;
    auto e = i2c_master_transmit_receive(device_, &reg, 1, &mode, 1, 50);
    if (e == ESP_OK && (mode & 0x30) != 0x20) e = ESP_ERR_INVALID_STATE;
    return e;
}
esp_err_t CogletController::InitHardware() {
    // Name the failing step: "PCA init: ESP_ERR_INVALID_STATE" alone cannot
    // distinguish a busy I2C port from a servo board that never answers.
    auto step = [](const char* what, esp_err_t err) {
        if (err != ESP_OK) ESP_LOGE("Coglet", "PCA init %s: %s", what, esp_err_to_name(err));
        return err;
    };
    // Assert OE BEFORE configuring the output direction; external pull-up is
    // required for the reset window. Existing Eyemech OE may be UNWIRED.
    auto e = step("oe_level", gpio_set_level(SERVO_OE_PIN, 1));
    if (e != ESP_OK) return e;
    if ((e = step("oe_dir", gpio_set_direction(SERVO_OE_PIN, GPIO_MODE_OUTPUT))) != ESP_OK) return e;
    i2c_master_bus_config_t bus = {};
    bus.i2c_port = SERVO_I2C_PORT;
    bus.sda_io_num = SERVO_I2C_SDA_PIN; bus.scl_io_num = SERVO_I2C_SCL_PIN;
    bus.clk_source = I2C_CLK_SRC_DEFAULT; bus.glitch_ignore_cnt = 7;
    bus.flags.enable_internal_pullup = true;
    if ((e = step("new_bus", i2c_new_master_bus(&bus, &bus_))) != ESP_OK) return e;
    i2c_device_config_t dev = {};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7; dev.device_address = SERVO_PCA9685_ADDR;
    dev.scl_speed_hz = 100000;
    if ((e = step("add_device", i2c_master_bus_add_device(bus_, &dev, &device_))) != ESP_OK) return e;
    // Full-off ALL outputs before waking/reprogramming a warm PCA9685.
    if ((e = step("release", Release())) != ESP_OK) return e;
    if ((e = Register(0, 0x30)) != ESP_OK) return e; // sleep + auto-increment
    if ((e = Register(0xfe, 121)) != ESP_OK) return e; // nominal 25 MHz, ~50 Hz
    if ((e = Register(1, 4)) != ESP_OK) return e;
    if ((e = Register(0, 0x20)) != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));
    return Register(0, 0xa0);
}
esp_err_t CogletController::Release() {
    released_ = true; builder_ = false; animation_ = -1; blink_until_ = 0;
    auto oe = gpio_set_level(SERVO_OE_PIN, 1);
    // Single-register full-off works even before auto-increment is configured.
    auto off = Register(0xfd, 0x10);
    commanded_.fill(NAN);
    release_error_ = oe != ESP_OK ? oe : off;
    return release_error_;
}
esp_err_t CogletController::Fail(esp_err_t error) {
    if (error != ESP_OK) { last_error_ = error; Release(); }
    return error;
}
bool CogletController::Valid(const Calibration& cal, bool complete) const {
    if (cal.version != 1 || !Unit(cal.lid_trim) || !Unit(cal.upper_coeff)) return false;
    unsigned used = 0;
    for (const auto& a : cal.axes) {
        if (a.confirmed > 1 || a.channel < -1 || a.channel > 5) return false;
        if (a.channel >= 0) {
            if (used & (1u << a.channel)) return false;
            used |= 1u << a.channel;
        }
        if (!std::isfinite(a.low) || !std::isfinite(a.high) ||
            a.low < 0 || a.low > 180 || a.high < 0 || a.high > 180 ||
            a.min_us < 300 || a.max_us > 3000 || a.min_us >= a.max_us ||
            a.trim_us < -2700 || a.trim_us > 2700 ||
            a.min_us + a.trim_us < 300 || a.max_us + a.trim_us > 3000) return false;
        if ((complete || a.confirmed) && (a.channel < 0 || a.low == a.high || !a.confirmed)) return false;
    }
    return true;
}
void CogletController::Load() {
    nvs_handle_t h;
    if (nvs_open("coglet", NVS_READONLY, &h) != ESP_OK) return;
    Calibration candidate;
    size_t size = sizeof(candidate);
    auto e = nvs_get_blob(h, "cal_v1", &candidate, &size);
    nvs_close(h);
    if (e == ESP_OK && size == sizeof(candidate) && Valid(candidate, false)) cal_ = candidate;
}
esp_err_t CogletController::Save() {
    nvs_handle_t h;
    auto e = nvs_open("coglet", NVS_READWRITE, &h);
    if (e != ESP_OK) return e;
    e = nvs_set_blob(h, "cal_v1", &cal_, sizeof(cal_));
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h); return e;
}
esp_err_t CogletController::Write(int axis, float degrees) {
    if (released_ || !ready_) return ESP_ERR_INVALID_STATE;
    if (axis < 0 || axis >= 6 || !std::isfinite(degrees)) return ESP_ERR_INVALID_ARG;
    const auto& a = cal_.axes[axis];
    if (a.channel < 0 || degrees < std::min(a.low,a.high) || degrees > std::max(a.low,a.high)) return ESP_ERR_INVALID_ARG;
    if (commanded_[axis] == degrees) return ESP_OK;
    float us = a.min_us + (a.max_us-a.min_us)*degrees/180.f + a.trim_us;
    // Use the actual programmed prescaler, rather than assuming exactly 20ms.
    int ticks = std::lround(us * 25.f / 122.f);
    uint8_t bytes[] = {uint8_t(6+4*a.channel), 0, 0, uint8_t(ticks), uint8_t(ticks>>8)};
    auto e = i2c_master_transmit(device_, bytes, sizeof(bytes), 50);
    if (e == ESP_OK) commanded_[axis] = degrees; // commit only after success
    return e;
}
esp_err_t CogletController::Apply(const std::array<float,4>& pose) {
    auto target = pose;
    // Eyemech upper-lid tracking: trim scales measured open end, then hood
    // by 0.8 * (1 - normalized vertical gaze). Coglet has no lower eyelids.
    float coupled = (0.5f + 0.5f*cal_.lid_trim)*(1.f-cal_.upper_coeff*(1.f-pose[1]));
    if (std::isnan(target[2])) target[2] = coupled;
    if (std::isnan(target[3])) target[3] = coupled;
    for (int i=0; i<4; ++i) {
        const auto& a=cal_.axes[i];
        auto e = Write(i, a.low+(a.high-a.low)*target[i]);
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}
esp_err_t CogletController::Tick(int64_t now) {
    if (released_) return ESP_OK;
    auto e = Probe(); // detects absent/reset PCA even if cached targets repeat
    if (e != ESP_OK) return e;
    if (builder_) return ESP_OK;
    if (animation_ >= 0) {
        const auto& a=animations[animation_]; const auto& f=a.frames[frame_];
        float k=std::min(1.f, float(now-frame_start_)/f.ms);
        float dest[]={f.lr,f.ud,f.lid_l,f.lid_r};
        auto next=pose_;
        for (int i=0;i<4;++i) {
            if (std::isnan(dest[i])) { if (i>=2) next[i]=NAN; }
            else next[i]=std::isnan(from_[i]) ? dest[i] : from_[i]+(dest[i]-from_[i])*k;
        }
        if ((e=Apply(next)) != ESP_OK) return e;
        pose_=next;
        if (k>=1) {
            if (++frame_ >= a.count) { animation_=-1; next_blink_=BlinkTime(now+900); }
            else { from_=pose_; frame_start_=now; }
        }
    } else if (blink_until_ && now>=blink_until_) {
        if ((e=Apply(pose_)) != ESP_OK) return e;
        blink_until_=0; next_blink_=BlinkTime(now+70);
    } else if (!blink_until_ && now>=next_blink_) {
        auto closed=pose_; closed[2]=closed[3]=0;
        if ((e=Apply(closed)) != ESP_OK) return e;
        blink_until_=now+70;
    }
    return ESP_OK;
}
cJSON* CogletController::State() {
    auto* root=cJSON_CreateObject();
    cJSON_AddBoolToObject(root,"released",released_);
    cJSON_AddBoolToObject(root,"calibrated",Valid(cal_,true));
    cJSON_AddBoolToObject(root,"builder",builder_);
    cJSON_AddBoolToObject(root,"pca_initialized",ready_);
    cJSON_AddStringToObject(root,"last_hardware_error",esp_err_to_name(last_error_));
    cJSON_AddStringToObject(root,"release_error",esp_err_to_name(release_error_));
    cJSON_AddStringToObject(root,"animation",animation_<0 ? "none" : animations[animation_].name);
    cJSON_AddStringToObject(root,"position_semantics","commanded positions; no feedback; null after release");
    cJSON_AddNumberToObject(root,"calibration_version",cal_.version);
    cJSON_AddNumberToObject(root,"lid_trim",cal_.lid_trim);
    cJSON_AddNumberToObject(root,"upper_coeff",cal_.upper_coeff);
    auto* axes=cJSON_AddArrayToObject(root,"axes");
    for (int i=0;i<6;++i) {
        auto* v=cJSON_CreateObject(); const auto& a=cal_.axes[i];
        cJSON_AddStringToObject(v,"role",names[i]); cJSON_AddNumberToObject(v,"channel",a.channel);
        cJSON_AddNumberToObject(v,"low",a.low); cJSON_AddNumberToObject(v,"high",a.high);
        cJSON_AddNumberToObject(v,"min_us",a.min_us); cJSON_AddNumberToObject(v,"max_us",a.max_us);
        cJSON_AddNumberToObject(v,"trim_us",a.trim_us); cJSON_AddBoolToObject(v,"confirmed",a.confirmed);
        if (std::isfinite(commanded_[i])) cJSON_AddNumberToObject(v,"commanded_degrees",commanded_[i]);
        else cJSON_AddNullToObject(v,"commanded_degrees");
        cJSON_AddItemToArray(axes,v);
    }
    return root;
}

std::string CogletController::Command(const std::string& text, bool local) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::istringstream in(text);
    std::string verb; in >> verb;
    auto end = [&]() { std::string extra; return !(in >> extra); };
    auto check = [&](esp_err_t e) {
        if (e != ESP_OK) throw std::runtime_error(esp_err_to_name(e));
    };
    auto hardware = [&](esp_err_t e) { check(Fail(e)); };
    if (verb == "state" || verb == "export") {
        if (!end()) check(ESP_ERR_INVALID_ARG);
        if (ready_) Fail(Probe());
        return Json(State());
    }
    if (verb == "release") {
        if (!end()) check(ESP_ERR_INVALID_ARG);
        check(Release()); return Json(State());
    }
    // Local diagnostic: who is actually on the servo I2C bus? A PCA9685 that
    // does not answer at SERVO_PCA9685_ADDR is either unpowered, on different
    // pins, or strapped to another address; only a sweep tells them apart.
    if (verb == "scan") {
        if (!local) check(ESP_ERR_INVALID_ARG);
        // Optional pin override, so a suspected SDA/SCL swap or a harness on
        // different pads can be ruled out without reflashing. This tears the
        // servo bus down; the board stays released until the next reboot.
        int sda = -1, scl = -1;
        if (in >> sda) { if (!(in >> scl) || !end()) check(ESP_ERR_INVALID_ARG); }
        if (sda >= 0) {
            Release();
            if (device_) { i2c_master_bus_rm_device(device_); device_ = nullptr; }
            if (bus_) { i2c_del_master_bus(bus_); bus_ = nullptr; }
            ready_ = false;
            i2c_master_bus_config_t rescan = {};
            rescan.i2c_port = SERVO_I2C_PORT;
            rescan.sda_io_num = decltype(rescan.sda_io_num)(sda);
            rescan.scl_io_num = decltype(rescan.scl_io_num)(scl);
            rescan.clk_source = I2C_CLK_SRC_DEFAULT; rescan.glitch_ignore_cnt = 7;
            rescan.flags.enable_internal_pullup = true;
            check(i2c_new_master_bus(&rescan, &bus_));
        }
        if (!bus_) check(ESP_ERR_INVALID_STATE);
        std::string found;
        for (int addr = 0x08; addr <= 0x77; ++addr) {
            if (i2c_master_probe(bus_, addr, 50) == ESP_OK) {
                char hex[8];
                snprintf(hex, sizeof(hex), "0x%02x", addr);
                if (!found.empty()) found += ", ";
                found += hex;
            }
        }
        char pins[96];
        snprintf(pins, sizeof(pins), "I2C port %d, SDA GPIO%d, SCL GPIO%d: ",
                 (int)SERVO_I2C_PORT,
                 sda >= 0 ? sda : (int)SERVO_I2C_SDA_PIN,
                 sda >= 0 ? scl : (int)SERVO_I2C_SCL_PIN);
        return std::string(pins) + (found.empty() ? "no devices responded" : found);
    }
    if (verb == "stop") {
        if (!end()) check(ESP_ERR_INVALID_ARG);
        // Freeze the successful commanded pose, including lids, mid-animation.
        for (int i=0;i<4;++i) if (std::isfinite(commanded_[i]) && cal_.axes[i].low != cal_.axes[i].high) {
            const auto& a=cal_.axes[i]; pose_[i]=(commanded_[i]-a.low)/(a.high-a.low);
        }
        animation_=-1; blink_until_=0; next_blink_=BlinkTime(Now()+900);
        for (int i=0;i<4;++i) if (!std::isfinite(commanded_[i])) next_blink_=INT64_MAX;
        hardware(Probe()); return Json(State());
    }
    if (verb == "engage" || verb == "builder") {
        if (!local || !end()) check(ESP_ERR_INVALID_ARG);
        if (!released_) check(ESP_ERR_INVALID_STATE);
        if (verb == "engage" && !Valid(cal_,true)) check(ESP_ERR_INVALID_STATE);
        hardware(Probe());
        // Clear old per-channel pulses while global full-off and OE remain set.
        for (int ch=0;ch<16;++ch) {
            uint8_t bytes[]={uint8_t(6+4*ch),0,0,0,0x10};
            hardware(i2c_master_transmit(device_,bytes,sizeof(bytes),50));
        }
        hardware(Register(0xfd,0));
        hardware(gpio_set_level(SERVO_OE_PIN,0));
        released_=false; builder_=verb=="builder"; last_error_=ESP_OK;
        // Engagement alone commands no pose; auto-blink starts after gaze/anim.
        next_blink_=INT64_MAX;
        return Json(State());
    }
    if (verb == "configure") {
        std::string role; int confirmed;
        Axis a;
        if (!local || !released_ || !(in>>role>>a.channel>>a.low>>a.high>>a.min_us>>a.max_us>>a.trim_us>>confirmed) || !end()) check(ESP_ERR_INVALID_ARG);
        int i=AxisIndex(role);
        if (i<0 || (confirmed!=0 && confirmed!=1)) check(ESP_ERR_INVALID_ARG);
        a.confirmed=confirmed; auto candidate=cal_; candidate.axes[i]=a;
        if (!Valid(candidate,false)) check(ESP_ERR_INVALID_ARG);
        cal_=candidate; return Json(State());
    }
    if (verb == "lids") {
        float trim, coeff;
        if (!local || !released_ || !(in>>trim>>coeff) || !end() || !Unit(trim) || !Unit(coeff)) check(ESP_ERR_INVALID_ARG);
        cal_.lid_trim=trim; cal_.upper_coeff=coeff; return Json(State());
    }
    if (verb == "save") {
        if (!local || !released_ || !end()) check(ESP_ERR_INVALID_STATE);
        check(Save()); return Json(State());
    }
    if (verb == "web") {
        std::string value;
        if (!local || !(in>>value) || !end() || (value!="on" && value!="off")) check(ESP_ERR_INVALID_ARG);
        web_enabled_=value=="on"; return Json(State());
    }
    if (verb == "servo" || verb == "jog") {
        std::string role; float angle;
        if (!local || !builder_ || released_ || !(in>>role>>angle) || !end() || !std::isfinite(angle)) check(ESP_ERR_INVALID_ARG);
        int i=AxisIndex(role); if (i<0) check(ESP_ERR_INVALID_ARG);
        if (verb=="jog") {
            if (std::abs(angle)>5 || !std::isfinite(commanded_[i])) check(ESP_ERR_INVALID_ARG);
            angle+=commanded_[i];
        }
        const auto& a=cal_.axes[i];
        if (a.channel<0 || angle<std::min(a.low,a.high) || angle>std::max(a.low,a.high)) check(ESP_ERR_INVALID_ARG);
        hardware(Probe()); hardware(Write(i,angle)); return Json(State());
    }
    if (verb != "gaze" && verb != "blink" && verb != "animate") check(ESP_ERR_NOT_SUPPORTED);
    if (released_ || builder_ || !Valid(cal_,true)) check(ESP_ERR_INVALID_STATE);
    if (verb == "gaze") {
        float x,y;
        if (!(in>>x>>y) || !end() || !std::isfinite(x) || !std::isfinite(y) || x< -100 || x>100 || y< -100 || y>100) check(ESP_ERR_INVALID_ARG);
        std::array<float,4> next={(x+100)/200,(y+100)/200,NAN,NAN};
        hardware(Probe()); hardware(Apply(next)); pose_=next;
        animation_=-1; blink_until_=0; next_blink_=BlinkTime(Now());
    } else if (verb == "blink") {
        if (!end()) check(ESP_ERR_INVALID_ARG);
        if (animation_>=0 || blink_until_) check(ESP_ERR_INVALID_STATE);
        // Blink only after a gaze/animation has established a commanded pose.
        for (int i=0;i<4;++i) if (!std::isfinite(commanded_[i])) check(ESP_ERR_INVALID_STATE);
        auto closed=pose_; closed[2]=closed[3]=0;
        hardware(Probe()); hardware(Apply(closed)); blink_until_=Now()+70;
    } else {
        std::string name; if (!(in>>name) || !end()) check(ESP_ERR_INVALID_ARG);
        int index=-1;
        for (int i=0;i<6;++i) if (name==animations[i].name) index=i;
        if (index<0) check(ESP_ERR_INVALID_ARG);
        hardware(Probe());
        // Establish the current logical pose synchronously so missing hardware
        // fails this MCP call. Remaining frames run on the device timer.
        hardware(Apply(pose_));
        animation_=index; frame_=0; from_=pose_; frame_start_=Now(); blink_until_=0;
    }
    auto* result=State();
    cJSON_AddStringToObject(result,"action_status",verb=="gaze" ? "commanded" : "started; poll state for later hardware faults");
    return Json(result);
}

void CogletController::Tools() {
    auto& m=McpServer::GetInstance();
    m.AddTool("self.coglet.gaze","Bounded base/tilt gaze (-100..100 within calibrated endpoints), with upper-lid tracking. Requires local engagement.",
        PropertyList({Property("horizontal",kPropertyTypeInteger,0,-100,100),Property("vertical",kPropertyTypeInteger,0,-100,100)}),
        [this](const PropertyList& p)->ReturnValue {return Command("gaze "+std::to_string(p["horizontal"].value<int>())+" "+std::to_string(p["vertical"].value<int>()));});
    m.AddTool("self.coglet.blink","Start one device-timed blink; requires established gaze and local engagement.",PropertyList(),
        [this](const PropertyList&)->ReturnValue{return Command("blink");});
    m.AddTool("self.coglet.animate","Start a device-timed expression: look, roll, side_eye, wink, surprise, sleepy. Poll state for completion or later faults.",
        PropertyList({Property("name",kPropertyTypeString)}),
        [this](const PropertyList& p)->ReturnValue{return Command("animate "+p["name"].value<std::string>());});
    m.AddTool("self.coglet.stop","Stop animation at the last commanded pose. Automatic blinking resumes after a settling interval; use release to prevent all motion.",PropertyList(),
        [this](const PropertyList&)->ReturnValue{return Command("stop");});
    m.AddTool("self.coglet.release","Latch all servo outputs off. Only local builder controls can engage again. This does not cut servo power.",PropertyList(),
        [this](const PropertyList&)->ReturnValue{return Command("release");});
    m.AddTool("self.coglet.state","Diagnostic state, calibration and commanded positions (no position feedback), release latch and hardware errors.",PropertyList(),
        [this](const PropertyList&)->ReturnValue{return Command("state");});
}

esp_err_t CogletController::Http(httpd_req_t* req) {
    auto& c=Instance();
    {
        std::lock_guard<std::mutex> lock(c.mutex_);
        if (!c.web_enabled_) return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Enable locally with !coglet web on");
    }
    if (req->method==HTTP_GET) {
        httpd_resp_set_type(req,"text/html");
        return httpd_resp_sendstr(req,R"HTML(<!doctype html><meta name="viewport" content="width=device-width"><title>Coglet builder</title>
<h1>Coglet builder controls</h1><p>Servos have no position feedback. Keep a hand on the servo-power switch.</p>
<p>Commands: state, export, release, builder, engage, servo ROLE DEGREES, jog ROLE DELTA, gaze X Y, blink, animate NAME, stop, save.</p>
<p>Configure while released: configure ROLE CHANNEL LOW HIGH MIN_US MAX_US TRIM_US CONFIRMED. Roles: base tilt lid_left lid_right mouth ears. Confirmed: 0 or 1.</p>
<form id="form"><input id="command" size="65" value="state"><button>Run</button></form>
<button onclick="run('release')">Release outputs</button> <button onclick="run('state')">State</button>
<pre id="result"></pre><script>
async function run(command) {try {let r=await fetch('/command',{method:'POST',headers:{'Content-Type':'text/plain'},body:command});let t=await r.text();document.getElementById('result').textContent=r.status+' '+t;}catch(e){document.getElementById('result').textContent=e;}}
document.getElementById('form').onsubmit=e=>{e.preventDefault();run(document.getElementById('command').value)};
</script>)HTML");
    }
    // No CORS; reject cross-origin browser commands and overly long bodies.
    if (httpd_req_get_hdr_value_len(req,"Origin")) {
        char origin[192], host[160];
        if (httpd_req_get_hdr_value_str(req,"Origin",origin,sizeof(origin))!=ESP_OK ||
            httpd_req_get_hdr_value_str(req,"Host",host,sizeof(host))!=ESP_OK ||
            std::string(origin)!="http://"+std::string(host))
            return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Origin mismatch");
    }
    if (req->content_len==0 || req->content_len>240) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Command length");
    std::string body(req->content_len,'\0'); size_t received=0;
    while (received<body.size()) {
        int n=httpd_req_recv(req,body.data()+received,body.size()-received);
        if (n<=0) return ESP_FAIL;
        received+=n;
    }
    try {
        auto result=c.Command(body,true);
        httpd_resp_set_type(req,"application/json"); return httpd_resp_sendstr(req,result.c_str());
    } catch(const std::exception& e) {return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,e.what());}
}
void CogletController::Web() {
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();
    config.server_port=8080; config.ctrl_port=32770; config.stack_size=6144;
    httpd_handle_t server=nullptr;
    auto e=httpd_start(&server,&config);
    if (e==ESP_OK) {
        httpd_uri_t root={}; root.uri="/";root.method=HTTP_GET;root.handler=Http;
        e=httpd_register_uri_handler(server,&root);
        if (e==ESP_OK) {root.uri="/command";root.method=HTTP_POST;e=httpd_register_uri_handler(server,&root);}
    }
    if (e!=ESP_OK) ESP_LOGE("Coglet","Browser controls unavailable: %s",esp_err_to_name(e));
}
void CogletController::Start() {
    commanded_.fill(NAN); Load();
    last_error_=InitHardware(); ready_=last_error_==ESP_OK;
    if (!ready_) {Release(); ESP_LOGE("Coglet","PCA init: %s",esp_err_to_name(last_error_));}
    Tools();
    if (xTaskCreate([](void*) {
        auto& c=Instance();
        for (;;) {
            {std::lock_guard<std::mutex> lock(c.mutex_); c.Fail(c.Tick(Now()));}
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    },"coglet_motion",4096,nullptr,4,nullptr)!=pdPASS) {
        ready_=false; Fail(ESP_ERR_NO_MEM);
    }
    // Wait for the application to initialize networking before HTTP startup.
    if (xTaskCreate([](void*) {
        while (!esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")) vTaskDelay(pdMS_TO_TICKS(500));
        Instance().Web(); vTaskDelete(nullptr);
    },"coglet_web",4096,nullptr,2,nullptr)!=pdPASS) ESP_LOGE("Coglet","Cannot start browser task");
}

bool CogletSerialCommand(const char* line) {
    if (strncmp(line,"!coglet",7) || (line[7] && line[7]!=' ')) return false;
    try {printf("\r\n%s\r\n",CogletController::Instance().Command(line[7]?line+8:"state",true).c_str());}
    catch(const std::exception& e) {printf("\r\nCoglet error: %s\r\n",e.what());}
    return true;
}
