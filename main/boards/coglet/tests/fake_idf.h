#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <initializer_list>
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_INVALID_STATE=1, ESP_ERR_INVALID_ARG=2,
 ESP_ERR_NO_MEM=3, ESP_ERR_NOT_SUPPORTED=4;
inline const char* esp_err_to_name(int e) {return e==0?"ESP_OK":"FAKE_ERROR";}
using i2c_master_bus_handle_t=void*;
using i2c_master_dev_handle_t=void*;
constexpr int I2C_NUM_1=1, I2C_CLK_SRC_DEFAULT=0,I2C_ADDR_BIT_LEN_7=0;
constexpr int GPIO_NUM_5=5,GPIO_NUM_6=6,GPIO_NUM_9=9,GPIO_MODE_OUTPUT=1;
struct i2c_master_bus_config_t {int i2c_port,sda_io_num,scl_io_num,clk_source,glitch_ignore_cnt;struct {bool enable_internal_pullup;} flags;};
struct i2c_device_config_t {int dev_addr_length,device_address,scl_speed_hz;};
inline int fake_fail_after=-1, fake_probe_error=0, fake_oe=1;
inline int64_t fake_now=0;
inline std::vector<std::vector<uint8_t>> fake_writes;
inline int i2c_new_master_bus(const i2c_master_bus_config_t* c,void** p) {*p=(void*)1; return c->i2c_port==1 && c->sda_io_num==5 && c->scl_io_num==6 ? 0:-1;}
inline int i2c_master_bus_add_device(void*,const i2c_device_config_t* c,void** p) {*p=(void*)1;return c->device_address==0x40?0:-1;}
inline int i2c_master_transmit(void*,const uint8_t* p,size_t n,int) {
    if (fake_fail_after==0) return ESP_FAIL;
    if (fake_fail_after>0) --fake_fail_after;
    fake_writes.emplace_back(p,p+n);return 0;
}
inline int i2c_master_transmit_receive(void*,const uint8_t*,size_t,uint8_t* out,size_t,int) {*out=0x20;return fake_probe_error;}
inline int gpio_set_level(int,int level) {fake_oe=level;return 0;}
inline int gpio_set_direction(int,int) {return 0;}
inline int64_t esp_timer_get_time() {return fake_now*1000;}
inline uint32_t esp_random() {return 500;}
inline void* esp_netif_get_handle_from_ifkey(const char*) {return (void*)1;}
#define ESP_LOGE(...) ((void)0)
#define pdMS_TO_TICKS(x) (x)
constexpr int pdPASS=1;
inline void vTaskDelay(int) {}
inline void vTaskDelete(void*) {}
inline int xTaskCreate(void(*)(void*),const char*,int,void*,int,void*) {return 1;}
using nvs_handle_t=int;
constexpr int NVS_READONLY=0,NVS_READWRITE=1;
inline std::vector<uint8_t> fake_nvs;
inline int nvs_open(const char*,int,int* h) {*h=1;return 0;}
inline void nvs_close(int) {}
inline int nvs_commit(int) {return 0;}
inline int nvs_set_blob(int,const char*,const void* p,size_t n) {auto b=(const uint8_t*)p;fake_nvs.assign(b,b+n);return 0;}
inline int nvs_get_blob(int,const char*,void* p,size_t* n) {if (fake_nvs.empty() || fake_nvs.size()>*n) return -1;*n=fake_nvs.size();memcpy(p,fake_nvs.data(),*n);return 0;}
constexpr int HTTP_GET=0,HTTP_POST=1,HTTPD_403_FORBIDDEN=403,HTTPD_400_BAD_REQUEST=400;
struct httpd_req_t {int method;size_t content_len;};
struct httpd_config_t {int server_port=80,ctrl_port=1,stack_size=4096;};
#define HTTPD_DEFAULT_CONFIG() httpd_config_t{}
using httpd_handle_t=void*;
struct httpd_uri_t {const char* uri;int method;int (*handler)(httpd_req_t*);};
inline int httpd_resp_send_err(httpd_req_t*,int,const char*) {return 0;}
inline int httpd_resp_set_type(httpd_req_t*,const char*) {return 0;}
inline int httpd_resp_sendstr(httpd_req_t*,const char*) {return 0;}
inline int httpd_req_get_hdr_value_len(httpd_req_t*,const char*) {return 0;}
inline int httpd_req_get_hdr_value_str(httpd_req_t*,const char*,char*,size_t) {return -1;}
inline int httpd_req_recv(httpd_req_t*,char*,size_t) {return -1;}
inline int httpd_start(void**,const httpd_config_t*) {return 0;}
inline int httpd_register_uri_handler(void*,const httpd_uri_t*) {return 0;}
using ReturnValue=std::string;
constexpr int kPropertyTypeInteger=0,kPropertyTypeString=1;
struct Property {template<class... T> Property(T...) {} template<class T>T value() const {return {};}};
struct PropertyList {PropertyList() {} PropertyList(std::initializer_list<Property>) {} Property operator[](const char*) const {return Property();}};
class McpServer {public: static McpServer& GetInstance() {static McpServer m;return m;} template<class F> void AddTool(const char*,const char*,PropertyList,F) {}};
