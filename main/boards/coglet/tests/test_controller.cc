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
    const char* roles[]={"base","tilt","lid_left","lid_right","mouth","ears"};
    for(int i=0;i<6;++i) {
        auto cmd="configure "+std::string(roles[i])+" "+std::to_string(i)+(i==2?" 90 20":" 40 140")+" 1000 2000 0 1";
        c.Command(cmd,true);
    }
    Reject([&]{c.Command("configure mouth 0 40 140 1000 2000 0 1",true);});
    Reject([&]{c.Command("configure mouth 4 nan 140 1000 2000 0 1",true);});
    Reject([&]{c.Command("configure mouth 4 40 140 1000 2000 2000 1",true);});
    c.Command("save",true); c.cal_.axes[0].channel=-1; c.Load(); assert(c.Valid(c.cal_,true));
    c.Command("engage",true);
    for(float a:c.commanded_) assert(std::isnan(a));
    fake_writes.clear(); c.Tick(100000); assert(fake_writes.empty()); // engage doesn't blink/move
    Reject([&]{c.Command("blink");});
    Reject([&]{c.Command("gaze 101 0");});
    c.Command("gaze 0 -100");
    assert(std::abs(c.commanded_[2]-(90+(20-90)*.925f*.2f))<.001f);
    assert(std::isnan(c.commanded_[4]) && std::isnan(c.commanded_[5]));
    c.Command("gaze 0 100"); assert(std::abs(c.commanded_[2]-(90+(20-90)*.925f))<.001f);
    c.Command("blink"); assert(c.commanded_[2]==90);
    c.Tick(70); assert(!c.blink_until_);
    // Each timed animation completes using only the first four role mappings.
    for(const char* name:{"look","roll","side_eye","wink","surprise","sleepy"}) {
        c.Command(std::string("animate ")+name);
        for(int i=0;i<1000 && c.animation_>=0;++i) {fake_now+=10;assert(c.Tick(fake_now)==0);}
        assert(c.animation_<0 && std::isnan(c.commanded_[4]) && std::isnan(c.commanded_[5]));
    }
    c.Command("animate roll"); c.Command("stop"); assert(c.animation_<0);
    // The low-level cache must not record failed writes.
    float before=c.commanded_[0]; fake_fail_after=0;
    assert(c.Write(0,before==60?61:60)!=0 && c.commanded_[0]==before);
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
    Reject([&]{c.Command("servo mouth 90");});
    c.Command("servo mouth 90",true);c.Command("jog mouth 2",true);
    Reject([&]{c.Command("jog mouth 10",true);});
    assert(c.commanded_[4]==92 && std::isnan(c.commanded_[0]));
    if (argc>1) std::ofstream(argv[1]) << c.Command("export");
    auto invalid=c.cal_; invalid.axes[0].confirmed=2; assert(!c.Valid(invalid,false));
    invalid=c.cal_; invalid.axes[0].trim_us=INT32_MAX; assert(!c.Valid(invalid,false));
    fake_nvs.assign(3,0xff);
    CogletController fresh; fresh.Load(); assert(!fresh.Valid(fresh.cal_,true));
    fake_fail_after=0; assert(fresh.InitHardware()!=ESP_OK && fresh.released_);
    std::cout<<"Coglet host safety and motion tests passed (fake I2C/NVS; no hardware).\n";
}
