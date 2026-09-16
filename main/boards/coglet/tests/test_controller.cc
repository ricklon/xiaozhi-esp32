#include <array>
#include <cmath>
#include <mutex>
#include <string>
#include <fake_idf.h>
#include "cJSON.h"
#define private public
#include "coglet_controller.h"
#undef private
#include <cassert>
#include <iostream>
#include <functional>
#include <fstream>
#include <cstdint>

static void Reject(const std::function<void()>& f) {
    bool rejected=false; try {f();} catch(...) {rejected=true;} assert(rejected);
}
int main(int argc, char** argv) {
    auto& c=CogletController::Instance();
    c.Start();
    assert(c.ready_ && c.released_ && fake_oe==1);
    // Warm-start sequence disables outputs before MODE1 writes; no PWM pose.
    assert(fake_writes.front()==std::vector<uint8_t>({0xfd,0x10}));
    for (const auto& w:fake_writes) assert(w.size()==2);
    Reject([&]{c.Command("gaze 0 0");});
    Reject([&]{c.Command("engage",true);});
    Reject([&]{c.Command("engage");});
    // Roles bottom-up; index 5 is the shared lid servo, whose window runs
    // backwards (closed to open).
    const char* roles[]={"base","neck_tilt","neck_roll","eye_pan","eye_tilt",
                         "lids","jaw","ear_left","ear_right"};
    for(int i=0;i<9;++i) {
        auto cmd="configure "+std::string(roles[i])+" "+std::to_string(i)+(i==5?" 90 20":" 40 140")+" 1000 2000 0 1";
        c.Command(cmd,true);
    }
    Reject([&]{c.Command("configure jaw 0 40 140 1000 2000 0 1",true);});
    Reject([&]{c.Command("configure jaw 6 nan 140 1000 2000 0 1",true);});
    Reject([&]{c.Command("configure jaw 6 40 140 1000 2000 2000 1",true);});
    c.Command("save",true); c.cal_.axes[0].channel=-1; c.Load(); assert(c.Valid(c.cal_,true));
    c.Command("engage",true);
    for(float a:c.commanded_) assert(std::isnan(a));
    fake_writes.clear(); c.Tick(100000); assert(fake_writes.empty()); // engage doesn't blink/move
    Reject([&]{c.Command("blink");});
    Reject([&]{c.Command("gaze 101 0");});
    c.Command("gaze 0 -100");
    assert(std::abs(c.commanded_[5]-(90+(20-90)*.925f*.2f))<.001f);
    // The neck, jaw and ears have no choreography and must stay put.
    for(int i:{0,1,2,6,7,8}) assert(std::isnan(c.commanded_[i]));
    c.Command("gaze 0 100"); assert(std::abs(c.commanded_[5]-(90+(20-90)*.925f))<.001f);
    c.Command("blink"); assert(c.commanded_[5]==90);
    c.Tick(70); assert(!c.blink_until_);
    // Each timed animation completes using only the three motion axes.
    for(const char* name:{"look","roll","side_eye","wink","surprise","sleepy"}) {
        c.Command(std::string("animate ")+name);
        for(int i=0;i<1000 && c.animation_>=0;++i) {fake_now+=10;assert(c.Tick(fake_now)==0);}
        assert(c.animation_<0);
        for(int i:{0,1,2,6,7,8}) assert(std::isnan(c.commanded_[i]));
    }
    c.Command("animate roll"); c.Command("stop"); assert(c.animation_<0);
    // The low-level cache must not record failed writes.
    float before=c.commanded_[3]; fake_fail_after=0;
    assert(c.Write(3,before==60?61:60)!=0 && c.commanded_[3]==before);
    Reject([&]{c.Command("gaze -100 0");});
    assert(c.released_ && c.last_error_!=0 && c.release_error_!=0);
    for(float a:c.commanded_) assert(std::isnan(a));
    fake_fail_after=-1; c.Command("release");
    auto count=fake_writes.size(); c.Tick(fake_now+100000);assert(fake_writes.size()==count);
    c.Command("engage",true); c.Command("gaze 0 0");
    // A disconnected PCA is detected even with identical cached targets.
    fake_probe_error=-1; Reject([&]{c.Command("gaze 0 0");});assert(c.released_);
    fake_probe_error=0; c.Command("engage",true); c.Command("animate look");
    fake_fail_after=0;c.Fail(c.Tick(fake_now+100));assert(c.released_ && c.animation_<0);
    fake_fail_after=-1;c.Command("release");c.Command("builder",true);
    Reject([&]{c.Command("servo jaw 90");});
    c.Command("servo jaw 90",true);c.Command("jog jaw 2",true);
    Reject([&]{c.Command("jog jaw 10",true);});
    assert(c.commanded_[6]==92 && std::isnan(c.commanded_[0]));
    if (argc>1) std::ofstream(argv[1]) << c.Command("export");
    auto invalid=c.cal_; invalid.axes[0].confirmed=2; assert(!c.Valid(invalid,false));
    invalid=c.cal_; invalid.axes[0].trim_us=INT32_MAX; assert(!c.Valid(invalid,false));
    fake_nvs.assign(3,0xff);
    CogletController fresh; fresh.Load(); assert(!fresh.Valid(fresh.cal_,true));
    fake_fail_after=0; assert(fresh.InitHardware()!=ESP_OK && fresh.released_);
    fake_fail_after=-1; // the InitHardware check above left I2C failing
    // Endpoint hunting: builder-only, local-only, bounded step, and a marked
    // endpoint drops confirmation until a human has watched it again.
    Reject([&]{c.Command("explore jaw");});
    Reject([&]{c.Command("explore lids junk",true);});
    c.Command("explore jaw",true);
    assert(c.exploring_==6 && c.explore_angle_==90 && std::isnan(c.commanded_[6]));
    Reject([&]{c.Command("nudge 10",true);});
    c.Command("nudge 5",true); assert(c.explore_angle_==95);
    c.Command("mark low",true);
    assert(c.cal_.axes[6].low==95 && !c.cal_.axes[6].confirmed);
    c.Command("nudge -5",true); c.Command("mark high",true);
    assert(c.cal_.axes[6].high==90);
    c.Command("confirm jaw",true);
    assert(c.cal_.axes[6].confirmed==1 && c.exploring_<0);
    Reject([&]{c.Command("mark low",true);});      // nothing being explored
    // Nudging cannot leave the hard pulse bound even with a huge request.
    c.Command("explore jaw",true);
    for(int i=0;i<60;++i) c.Command("nudge -5",true);
    assert(c.explore_angle_==0);
    for(int i=0;i<80;++i) c.Command("nudge 5",true);
    assert(c.explore_angle_==180);
    // identify wiggles a raw channel that no role owns yet.
    fake_writes.clear();
    c.Command("identify 11",true);
    assert(!fake_writes.empty() && fake_writes.front().front()==6+4*11);
    Reject([&]{c.Command("identify 16",true);});
    Reject([&]{c.Command("identify 11");});
    // A partly built mechanism: base and tilt fitted, no lids yet.
    c.Command("release"); c.Command("confirm jaw",true);
    auto full=c.cal_;
    // No lid servo fitted yet: gaze still works, blink has nothing to move.
    c.cal_.axes[5].channel=-1; c.cal_.axes[5].confirmed=0;
    assert(c.Valid(c.cal_,true));
    c.Command("engage",true); c.Command("gaze 0 0");
    assert(std::isnan(c.commanded_[5]));
    Reject([&]{c.Command("blink");});
    assert(c.next_blink_==INT64_MAX);
    c.Command("release");
    // Bottom-up: base fitted but no eyes yet. Engaging is fine; gaze is not.
    c.cal_=full;
    for(int i:{3,4,5}) {c.cal_.axes[i].channel=-1; c.cal_.axes[i].confirmed=0;}
    assert(c.Valid(c.cal_,true));
    c.Command("engage",true);
    Reject([&]{c.Command("gaze 0 0");});
    Reject([&]{c.Command("animate look");});
    // Nothing fitted at all is not something to engage.
    auto bare=c.cal_;
    for(auto& a:bare.axes) {a.channel=-1; a.confirmed=0;}
    assert(!c.Valid(bare,true));
    // An assigned but unconfirmed role blocks engagement.
    auto lame=full; lame.axes[7].confirmed=0; assert(!c.Valid(lame,true));
    c.Command("release"); c.cal_=full;
    std::cout<<"Coglet host safety and motion tests passed (fake I2C/NVS; no hardware).\n";
}
