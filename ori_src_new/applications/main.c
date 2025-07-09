#include <rtthread.h>
#include <dfs_fs.h>
#include <wlan_mgnt.h>
#include <webclient.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <rtdevice.h>
#include <board.h>
#include "cJSON.h"
#include <math.h>
#include <drv_lcd.h>
#include <rttlogo.h>
#include "precess_data/precess_data.h"
#include "audio/audio.h"
#include "tmep_humi/app_temp_humi.h"
#include "webnet1/wn_sample.h"

#define RTC_NAME       "rtc"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 配置 KEY 输入引脚  */
#define KEY_UP_NUM          GET_PIN(C,  5)
#define KEY_DOWN_NUM        GET_PIN(C,  1)
#define KEY_LEFT_NUM        GET_PIN(C,  0)
#define KEY_RIGHT_NUM       GET_PIN(C,  4)

/* 配置 LED 灯引脚 */
#define PIN_LED_B              GET_PIN(F, 11)      // PF11 :  LED_B        --> LED
#define PIN_LED_R              GET_PIN(F, 12)      // PF12 :  LED_R        --> LED

/* 定义 LED 亮灭电平 */
#define LED_ON  (0)
#define LED_OFF (1)

#define HTTP_POST_URL "http://192.168.43.91:4000/api/upload"

static int onboard_sdcard_mount();
static int init_lcd();
static int rtc_lcd();
static int init_lcd_key();
static int init_lcd_pred();
static int init_lcd_wifi();

int driver_lcd();
int driver_lcd_enlarged2();
int thread_pin();
int store_float_array_from_json(const char *buffer, int group_number, int prediction_time);
int process_count();
int process_count_depths();
int process_count_enlarged();
int lcd_show_string_vertical();
int webclient_post_file(const char *URI, const char *filename, const char *form_data);
void assign_values();

static struct rt_semaphore net_ready;

#define MAX_OUTPUTS 370
float output_values[MAX_OUTPUTS]; // 全局数组存储float值

#define pred_OUTPUTS 9
float pred_nodes1[pred_OUTPUTS] = {0};
float pred_nodes2[pred_OUTPUTS] = {0};
float pred_nodes3[pred_OUTPUTS] = {0};
float pred_nodes4[pred_OUTPUTS] = {0};
float pred_nodes5[pred_OUTPUTS] = {0};
float pred_nodes6[pred_OUTPUTS] = {0};
float pred_nodes7[pred_OUTPUTS] = {0};
float pred_nodes8[pred_OUTPUTS] = {0};
float pred_nodes9[pred_OUTPUTS] = {0};
float pred_nodes10[pred_OUTPUTS] = {0};

#define jams_OUTPUTS 10
int jams[jams_OUTPUTS] = {0};

//#define nodes_OUTPUTS 180
int  output_nodes[nodes_OUTPUTS];
float  output_speed[nodes_OUTPUTS];
float output_aver_speed = 0;
float output_total_speed = 0;

unsigned int count = 0;
unsigned int last_count = -1; // 初始值为 -1，确保第一次处理时必然不同
unsigned int count_depths = 5;
unsigned int pred_depths = 1;
unsigned int last_count_depths = -1;
unsigned int count_enlarged = 0;
unsigned int last_count_enlarged = 0;
unsigned int count_add0 = 1;
unsigned int count_add1 = 1;
unsigned int count_add2 = 1;
int WorkSwitch = 1;



int Speed_Anomaly_Number = 0;
int no_congestion_nodes_numbers = 0;
int mid_congestion_nodes_numbers = 0;
int moderate_congestion_nodes_numbers = 0;
int severe_congestion_nodes_numbers = 0;


char mild_congestion[256] = "";
char moderate_congestion[256] = "";
char severe_congestion[256] = "";
char buffer_LCD_show[256] = {0};
char temp[10] = {0};
char temp_zhenshi[50] = {0};

extern float humidity;
extern float temperature;


char buffer_publish[1024] = {0};
char buffer_publish1[128] = {0};

int line = 0;
int total_lines = 55;  // 数据总行数

unsigned int auto_count_add = 0;

extern void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter);
extern void wlan_station_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter);

#include "rtthread.h"
#include "dev_sign_api.h"
#include "mqtt_api.h"

char DEMO_PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char DEMO_DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEMO_DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

void *HAL_Malloc(uint32_t size);
void HAL_Free(void *ptr);
void HAL_Printf(const char *fmt, ...);
int HAL_GetProductKey(char product_key[IOTX_PRODUCT_KEY_LEN + 1]);
int HAL_GetDeviceName(char device_name[IOTX_DEVICE_NAME_LEN + 1]);
int HAL_GetDeviceSecret(char device_secret[IOTX_DEVICE_SECRET_LEN]);
uint64_t HAL_UptimeMs(void);
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

static void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg);
static int example_subscribe(void *handle);
static int example_publish(void *handle);
static void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg);

#define EXAMPLE_TRACE(fmt, ...)  \
    do { \
        HAL_Printf("%s|%03d :: ", __func__, __LINE__); \
        HAL_Printf(fmt, ##__VA_ARGS__); \
        HAL_Printf("%s", "\r\n"); \
    } while(0)


int main(void)
{
    int result = RT_EOK;//   wifi join RedmiK70 123456789

    onboard_sdcard_mount();//挂载TF卡

    init_lcd();           //LCD初始化显示

    rtc_lcd();           //rtc初始化显示

    thread_pin();

    app_temp_humi_init();

    app_temp_humi_entry();



    result = rt_sem_init(&net_ready, "net_ready", 0, RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        return -RT_ERROR;
    }

    /* 注册 wlan 连接网络成功的回调，wlan 连接网络成功后释放 'net_ready' 信号量 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wlan_ready_handler, RT_NULL);
    /* 注册 wlan 网络断开连接的回调 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_DISCONNECTED, wlan_station_disconnect_handler, RT_NULL);
    LOG_D("-----等待 wlan 连接网络成功-----");
    /* 等待 wlan 连接网络成功 */
    result = rt_sem_take(&net_ready, RT_WAITING_FOREVER);
    if (result != RT_EOK)
    {
        LOG_E("Wait net ready failed!");
        rt_sem_delete(&net_ready);
        return -RT_ERROR;
    }
    LOG_D("-----网络连接成功-----");

    init_lcd_wifi();

    rt_thread_mdelay(1000);

    init_lcd_key();           //LCD功能展示




    /*while (0)
    {

        if (count != last_count)
            {
                // 使用 memset 将所有元素设置为0
                memset(output_nodes, 0, sizeof(output_nodes));
                process_count(count);
                last_count = count; // 更新 last_count 为当前的 count 值
                count_depths = 5;
                count_enlarged = 0;
                last_count_enlarged = 0;
                count_add = 1;
            }

//        if (count_depths != last_count_depths)
//            {
//                // 使用 memset 将所有元素设置为0
//                memset(output_nodes, 0, sizeof(output_nodes));
//                process_count_depths(count_depths);
//                last_count_depths = count_depths;
//                count_enlarged = 0;
//                count_add = 1;
//            }

        if (count_depths<45 && auto_count_add == 1)
            {
                // 使用 memset 将所有元素设置为0
                LOG_D("-----自动推理-----");
                count_depths = count_depths + 5;
                memset(output_nodes, 0, sizeof(output_nodes));
                process_count_depths(count_depths);
                count_enlarged = 0;
                last_count_enlarged = 0;
                count_add = 1;
                if(count_depths == 45){
                    auto_count_add = auto_count_add ^ 1;
                }
            }

        if (count_enlarged != last_count_enlarged)
            {
                process_count_enlarged(count_enlarged);
                last_count_enlarged = count_enlarged;
                rt_thread_mdelay(500);
                count_add = 1;
            }

    }*/

    //LOG_D("-----结束-----");

}

int process_count_depths(int count_depths)
{
    if (count_depths > 5){
        init_lcd_pred();
        webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_228.npy", "name=\"file\"; filename=\"PeMSD7_V_228.npy\"");
        driver_lcd(count,count_depths);
    }
    return RT_EOK;
}

int process_count_enlarged(int count_enlarged)
{
    switch (count_enlarged)
        {
            case 0:
                driver_lcd(count,count_depths);
                break;
            case 1:
                driver_lcd_enlarged2(count,count_depths);
                break;
            default:
                // Handle other cases or do nothing
                break;
        }
    return RT_EOK;
}

int process_count(int count)
{
    switch (count)
    {
        case 1:
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_1_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_1_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 2:
            // Handle case 2
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_2_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_2_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 3:
            // Handle case 3
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_3_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_3_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 4:
            // Handle case 4
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_4_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_4_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 5:
            // Handle case 5
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_5_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_5_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 6:
            // Handle case 6
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_6_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_6_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 7:
            // Handle case 7
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_7_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_7_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 8:
            // Handle case 8
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_8_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_8_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 9:
            // Handle case 9
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_9_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_9_21.xlsx\"");
            driver_lcd(count,5);
            break;
        case 10:
            // Handle case 10
            init_lcd_pred();
            webclient_post_file(HTTP_POST_URL, "/sdcard/PeMSD7_V_10_21.xlsx", "name=\"file\"; filename=\"PeMSD7_V_10_21.xlsx\"");
            driver_lcd(count,5);
            break;
        default:
            // Handle other cases or do nothing
            break;
    }
    return RT_EOK;
}

void assign_values(int pred_depths, float output_values[MAX_OUTPUTS], float pred_nodes1[MAX_OUTPUTS], float pred_nodes2[MAX_OUTPUTS], float pred_nodes3[MAX_OUTPUTS], float pred_nodes4[MAX_OUTPUTS], float pred_nodes5[MAX_OUTPUTS], float pred_nodes6[MAX_OUTPUTS], float pred_nodes7[MAX_OUTPUTS], float pred_nodes8[MAX_OUTPUTS], float pred_nodes9[MAX_OUTPUTS], float pred_nodes10[MAX_OUTPUTS]) {
    int i;
    switch (pred_depths) {
        case 1:
            pred_nodes1[0] = output_values[0];
            pred_nodes2[0] = output_values[1];
            pred_nodes3[0] = output_values[2];
            pred_nodes4[0] = output_values[3];
            pred_nodes5[0] = output_values[4];
            pred_nodes6[0] = output_values[5];
            pred_nodes7[0] = output_values[6];
            pred_nodes8[0] = output_values[7];
            pred_nodes9[0] = output_values[8];
            pred_nodes10[0] = output_values[9];
            // 将pred_nodes1到pred_nodes10数组的第2-8位赋值为0
            for (i = 1; i < 9; i++) {
                pred_nodes1[i] = 0;
                pred_nodes2[i] = 0;
                pred_nodes3[i] = 0;
                pred_nodes4[i] = 0;
                pred_nodes5[i] = 0;
                pred_nodes6[i] = 0;
                pred_nodes7[i] = 0;
                pred_nodes8[i] = 0;
                pred_nodes9[i] = 0;
                pred_nodes10[i] = 0;
            }
            break;
        case 2:
            pred_nodes1[1] = output_values[0];
            pred_nodes2[1] = output_values[1];
            pred_nodes3[1] = output_values[2];
            pred_nodes4[1] = output_values[3];
            pred_nodes5[1] = output_values[4];
            pred_nodes6[1] = output_values[5];
            pred_nodes7[1] = output_values[6];
            pred_nodes8[1] = output_values[7];
            pred_nodes9[1] = output_values[8];
            pred_nodes10[1] = output_values[9];
            break;
        // Extend for cases 3 to 8
        case 3:
            pred_nodes1[2] = output_values[0];
            pred_nodes2[2] = output_values[1];
            pred_nodes3[2] = output_values[2];
            pred_nodes4[2] = output_values[3];
            pred_nodes5[2] = output_values[4];
            pred_nodes6[2] = output_values[5];
            pred_nodes7[2] = output_values[6];
            pred_nodes8[2] = output_values[7];
            pred_nodes9[2] = output_values[8];
            pred_nodes10[2] = output_values[9];
            break;
        case 4:
            pred_nodes1[3] = output_values[0];
            pred_nodes2[3] = output_values[1];
            pred_nodes3[3] = output_values[2];
            pred_nodes4[3] = output_values[3];
            pred_nodes5[3] = output_values[4];
            pred_nodes6[3] = output_values[5];
            pred_nodes7[3] = output_values[6];
            pred_nodes8[3] = output_values[7];
            pred_nodes9[3] = output_values[8];
            pred_nodes10[3] = output_values[9];
            break;
        case 5:
            pred_nodes1[4] = output_values[0];
            pred_nodes2[4] = output_values[1];
            pred_nodes3[4] = output_values[2];
            pred_nodes4[4] = output_values[3];
            pred_nodes5[4] = output_values[4];
            pred_nodes6[4] = output_values[5];
            pred_nodes7[4] = output_values[6];
            pred_nodes8[4] = output_values[7];
            pred_nodes9[4] = output_values[8];
            pred_nodes10[4] = output_values[9];
            break;
        case 6:
            pred_nodes1[5] = output_values[0];
            pred_nodes2[5] = output_values[1];
            pred_nodes3[5] = output_values[2];
            pred_nodes4[5] = output_values[3];
            pred_nodes5[5] = output_values[4];
            pred_nodes6[5] = output_values[5];
            pred_nodes7[5] = output_values[6];
            pred_nodes8[5] = output_values[7];
            pred_nodes9[5] = output_values[8];
            pred_nodes10[5] = output_values[9];
            break;
        case 7:
            pred_nodes1[6] = output_values[0];
            pred_nodes2[6] = output_values[1];
            pred_nodes3[6] = output_values[2];
            pred_nodes4[6] = output_values[3];
            pred_nodes5[6] = output_values[4];
            pred_nodes6[6] = output_values[5];
            pred_nodes7[6] = output_values[6];
            pred_nodes8[6] = output_values[7];
            pred_nodes9[6] = output_values[8];
            pred_nodes10[6] = output_values[9];
            break;
        case 8:
            pred_nodes1[7] = output_values[0];
            pred_nodes2[7] = output_values[1];
            pred_nodes3[7] = output_values[2];
            pred_nodes4[7] = output_values[3];
            pred_nodes5[7] = output_values[4];
            pred_nodes6[7] = output_values[5];
            pred_nodes7[7] = output_values[6];
            pred_nodes8[7] = output_values[7];
            pred_nodes9[7] = output_values[8];
            pred_nodes10[7] = output_values[9];
            break;
        case 9:
            pred_nodes1[8] = output_values[0];
            pred_nodes2[8] = output_values[1];
            pred_nodes3[8] = output_values[2];
            pred_nodes4[8] = output_values[3];
            pred_nodes5[8] = output_values[4];
            pred_nodes6[8] = output_values[5];
            pred_nodes7[8] = output_values[6];
            pred_nodes8[8] = output_values[7];
            pred_nodes9[8] = output_values[8];
            pred_nodes10[8] = output_values[9];
            break;
        default:
            printf("Invalid pred_depths value\n");
    }
}

static int onboard_sdcard_mount(void)
{
    if (dfs_mount("sd0", "/sdcard", "elm", 0, 0) == RT_EOK)
    {
        LOG_I("SD card mount to '/sdcard'");
    }
    else
    {
        LOG_E("SD card mount to '/sdcard' failed!");
    }

    return RT_EOK;
}

int lcd_show_string_vertical(int x, int y, int font_size, const char *str)
{
    while (*str)
    {
        lcd_show_string(x, y, font_size, (char[]){*str, '\0'});
        y += font_size; // 改变Y坐标，向下移动
        str++;
    }
    return RT_EOK;
}

static int init_lcd(void)
{
    // LCD初始化
    lcd_clear(WHITE);

    /* show RT-Thread logo */
    lcd_show_image(0, 0, 240, 69, image_rttlogo);

    /* set the background color and foreground color */
    lcd_set_color(WHITE, BLACK);

    /* show some string on lcd */
    lcd_show_string(10, 69, 16, "Hello, RT-Thread!");
    lcd_show_string(10, 69 + 16, 24, "RT-Thread");
    lcd_show_string(10, 69 + 16 + 24, 32, "RT-Thread");

    /* draw a line on lcd */
    lcd_draw_line(0, 69 + 16 + 24 + 32, 240, 69 + 16 + 24 + 32);

    /* draw a concentric circles */
    lcd_draw_point(120, 194);

    for (int i = 0; i < 46; i += 4)
    {
        lcd_draw_circle(120, 194, i);
    }

    return RT_EOK;
}

static int rtc_lcd(void)
{
    rt_err_t ret = RT_EOK;
    time_t now;
    rt_device_t device = RT_NULL;

    /* 寻找设备 */
    device = rt_device_find(RTC_NAME);
    if (!device)
    {
        rt_kprintf("find %s failed!", RTC_NAME);
        return RT_ERROR;
    }

    /* 初始化RTC设备 */
    if(rt_device_open(device, 0) != RT_EOK)
    {
        rt_kprintf("open %s failed!", RTC_NAME);
        return RT_ERROR;
    }

    /* 设置日期 */
    ret = set_date(2024, 8, 12);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC date failed\n");
        return ret;
    }

    /* 设置时间 */
    ret = set_time(9, 15, 50);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC time failed\n");
        return ret;
    }

    /* 获取时间 */
    now = time(RT_NULL);
    rt_kprintf("%s\n", ctime(&now));

    return ret;
}

static int init_lcd_key(void)
{
    // LCD初始化
    lcd_clear(WHITE);

    lcd_set_color(WHITE, BLACK);

    /* show some string on lcd */
    lcd_show_string(10, 10, 16, "Left  Key:Previous set of");
    lcd_show_string(90, 10+16, 16, "speed data");
    lcd_show_string(10, 70, 16, "Right Key:Next set of");
    lcd_show_string(90, 70+16, 16, "speed data");
    lcd_show_string(10, 130, 16,"Up    Key:Traffic congestion");
    lcd_show_string(90, 130+16, 16, "analysis");
    lcd_show_string(10, 190, 16,"Down  Key:Increase inference");
    lcd_show_string(90, 190+16, 16, "depth");

    return RT_EOK;
}

static int init_lcd_pred(void)
{
    // LCD初始化
    lcd_clear(WHITE);

    lcd_set_color(WHITE, BLACK);

    /* show some string on lcd */
    lcd_show_string(20, 100, 32, "Predicting...");

    return RT_EOK;
}

static int init_lcd_wifi(void)
{
    // LCD初始化
    lcd_clear(WHITE);

    lcd_set_color(WHITE, BLACK);

    /* show some string on lcd */
    lcd_show_string(50, 60, 32, "Network  ");
    lcd_show_string(50, 100, 32, "connection ");
    lcd_show_string(50, 140, 32, "successful");

    return RT_EOK;
}

int driver_lcd(int group_number,int prediction_time)
{
    // LCD初始化
    lcd_clear(WHITE);

    LOG_D("\n-----生成速度预测图-----");

    lcd_set_color(WHITE, BLACK);

    /* draw a line on lcd */
    lcd_draw_line(40, 200, 230, 200);
    lcd_draw_line(215, 190, 230, 200);
    lcd_draw_line(215, 210, 230, 200);

    /* draw a line on lcd */
    lcd_draw_line(40, 5, 40, 200);
    lcd_draw_line(40, 5, 30, 20);
    lcd_draw_line(40, 5, 50, 20);

    /* show some string on lcd */
    lcd_show_string(30, 202, 16, "0");
    lcd_show_string(70, 202, 16, "40");
    lcd_show_string(110, 202, 16, "80");
    lcd_show_string(150, 202, 16, "120");
    lcd_show_string(190, 202, 16, "160");
    lcd_show_string(100, 220, 16, "Nodes");

    char buffer[50];
    sprintf(buffer, "Group %d---Pred %d mins", group_number, prediction_time);
    lcd_show_string(50, 0, 16, buffer);

    lcd_show_string(20, 149, 16, "20");
    lcd_show_string(20, 106, 16, "40");
    lcd_show_string(20, 63, 16, "60");
    lcd_show_string(20, 20, 16, "80");

    // 显示纵向字符串
    lcd_show_string_vertical(5, 20, 16, "Speed(km/h)");

    /* set the background color and foreground color *///   wifi join RedmiK70 123456789
    lcd_set_color(WHITE, BLACK);

    int n = 0;
    // 绘制数据曲线
    for (int i = 0; i < 184; i++)
    {
        // 计算数据点在屏幕上的坐标
        int x1 = i + 41;
        int x2 = i + 1 + 41;
        int y1 = (int)round(200 - (output_values[n + 185] * 2.15));
        int y2 = (int)round(200 - (output_values[n + 1 + 185] * 2.15));
        n = n + 1;

        // 绘制线段
        lcd_draw_line(x1, y1, x2, y2);
    }

    /* set the background color and foreground color *///   wifi join RedmiK70 123456789
    lcd_set_color(WHITE, RED);

    // 绘制数据曲线
    for (int i = 0; i < 184; i++)
    {
        // 计算数据点在屏幕上的坐标
        int x1 = i + 41;
        int x2 = i + 1 + 41;
        int y1 = (int)round(200 - (output_values[i] * 2.15));
        int y2 = (int)round(200 - (output_values[i + 1] * 2.15));

        // 绘制线段
        lcd_draw_line(x1, y1, x2, y2);
    }

    return RT_EOK;
}


/* 原始*/

//int driver_lcd_enlarged2(int group_number,int prediction_time)
//{
//    // LCD初始化
//    lcd_clear(WHITE);
//
//    lcd_set_color(WHITE, BLACK);//   wifi join RedmiK70 123456789
//
//    char buffer[256];
//    char mild_congestion[256] = "";
//    char moderate_congestion[256] = "";
//    char severe_congestion[256] = "";
//    int all_zeros = 1; // 标志变量，假设所有值均为0
//
//    // 构建包含output_nodes值的字符串
//    for (int i = 0; i < nodes_OUTPUTS; i++) {
//        if (output_nodes[i] != 0) {
//            all_zeros = 0; // 发现不为0的值，将标志变量置为0
//            char temp[10];
//            if (i == nodes_OUTPUTS - 1) {
//                sprintf(temp, "%d", output_nodes[i]);
//            } else {
//                sprintf(temp, "%d,", output_nodes[i]);
//            }
//
//            if (output_speed[i] > 30 && output_speed[i] <= 45) {
//                strcat(mild_congestion, temp);
//            } else if (output_speed[i] > 20 && output_speed[i] <= 30) {
//                strcat(moderate_congestion, temp);
//            } else if (output_speed[i] > 0 && output_speed[i] <= 20) {
//                strcat(severe_congestion, temp);
//            }
//        }
//    }
//
//    // 构建最终的字符串
//    if (all_zeros) {
//        snprintf(buffer, sizeof(buffer), "In the next %d mins, ", prediction_time);
//        lcd_show_string(0, 10, 16, buffer);
//        lcd_show_string(0, 30, 16, "good traffic is expected");
//        LOG_D("good traffic is expected");
//    } else {
//        snprintf(buffer, sizeof(buffer), "In the next %d mins, ", prediction_time);
//        lcd_show_string(0, 0, 16, buffer);
//
//        if (strlen(mild_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", mild_congestion);
//            lcd_show_string(0, 16, 16, "Nodes with mild congestion");
//            lcd_show_string(0, 32, 16, buffer);
//            printf("Nodes with mild congestion: %s\n", mild_congestion);
//        }
//
//        if (strlen(moderate_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", moderate_congestion);
//            lcd_show_string(0, 96, 16, "Nodes with moderate congestion");
//            lcd_show_string(0, 112, 16, buffer);
//            printf("Nodes with moderate congestion: %s\n", moderate_congestion);
//        }
//
//        if (strlen(severe_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", severe_congestion);
//            lcd_show_string(0, 176, 16, "Nodes with severe congestion");
//            lcd_show_string(0, 192, 16, buffer);
//            printf("Nodes with severe congestion: %s\n", severe_congestion);
//        }
//    }
//
////    printf("buffer: %s\n", buffer);
//
//    return RT_EOK;
//}

/* 修改版本1*/

//int driver_lcd_enlarged2(int group_number,int prediction_time)
//{
//    // LCD初始化
//    lcd_clear(WHITE);
//
//    lcd_set_color(WHITE, BLACK);//   wifi join RedmiK70 123456789
//
//    char buffer[256];
//    char mild_congestion[256] = "";
//    char moderate_congestion[256] = "";
//    char severe_congestion[256] = "";
//    char free_flow[256] = "";
//    int all_zeros = 1; // 标志变量，假设所有值均为0
//    float cal_congestion = 0;
//    int No = 0;
//
//    no_congestion_nodes_numbers = 0;
//    mid_congestion_nodes_numbers = 0;
//    moderate_congestion_nodes_numbers = 0;
//    severe_congestion_nodes_numbers = 0;
//
//    // 构建包含output_nodes值的字符串
//    for (int i = 0; i < nodes_OUTPUTS; i++) {
//        if (output_nodes[i] != 0) {
//            all_zeros = 0; // 发现不为0的值，将标志变量置为0
//            char temp[10];
//            if (i == nodes_OUTPUTS - 1)
//            {
//                sprintf(temp, "%d", output_nodes[i]);
//            }
//            else
//            {
//                sprintf(temp, "%d,", output_nodes[i]);
//            }
//
//            if (output_speed[i] > 45)
//            {
//                strcat(free_flow, temp);
//                no_congestion_nodes_numbers++;
//            }
//
//            if (output_speed[i] > 30 && output_speed[i] <= 45)
//            {
//                strcat(mild_congestion, temp);
//                mid_congestion_nodes_numbers++;
//            }
//            else if (output_speed[i] > 20 && output_speed[i] <= 30)
//            {
//                strcat(moderate_congestion, temp);
//                moderate_congestion_nodes_numbers++;
//            }
//            else if (output_speed[i] > 0 && output_speed[i] <= 20)
//            {
//                strcat(severe_congestion, temp);
//                severe_congestion_nodes_numbers++;
//            }
//        }
//    }
//    cal_congestion = Cal_Congestion_Degree(no_congestion_nodes_numbers, mid_congestion_nodes_numbers, moderate_congestion_nodes_numbers, severe_congestion_nodes_numbers);
//    No = Cal_Congestion_Degree_NO(cal_congestion);
//    // 构建最终的字符串
//    if (all_zeros)
//    {
//        snprintf(buffer, sizeof(buffer), "In the next %d mins, ", prediction_time);
//        lcd_show_string(0, 10, 16, buffer);
//        lcd_show_string(0, 30, 16, "good traffic is expected");
//        LOG_D("good traffic is expected");
//    }
//    else
//    {
//        snprintf(buffer, sizeof(buffer), "In the next %d mins, ", prediction_time);
//        lcd_show_string(0, 0, 16, buffer);
//
//        if (strlen(mild_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", mild_congestion);
//            lcd_show_string(0, 16, 16, "Nodes with mild congestion");
//            lcd_show_string(0, 32, 16, buffer);
//            printf("Nodes with mild congestion: %s\n", mild_congestion);
//        }
//
//        if (strlen(moderate_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", moderate_congestion);
//            lcd_show_string(0, 80, 16, "Nodes with moderate congestion");
//            lcd_show_string(0, 96, 16, buffer);
//            printf("Nodes with moderate congestion: %s\n", moderate_congestion);
//        }
//
//        if (strlen(severe_congestion) > 0) {
//            snprintf(buffer, sizeof(buffer), "%s", severe_congestion);
//            lcd_show_string(0, 144, 16, "Nodes with severe congestion");
//            lcd_show_string(0, 160, 16, buffer);
//            printf("Nodes with severe congestion: %s\n", severe_congestion);
//        }
//        lcd_show_string(0, 184, 16, "Road intersection congestion ratio​​");
//        lcd_show_num(0,200,cal_congestion,sizeof(float),16);
////        wavplay_enable(No);
//
//    }
//
////    printf("buffer: %s\n", buffer);
//
//    return RT_EOK;
//}



int driver_lcd_enlarged2(int group_number,int prediction_time)
{
    // LCD初始化
    lcd_clear(WHITE);

    lcd_set_color(WHITE, BLACK);//   wifi join RedmiK70 123456789


    memset(mild_congestion, 0, sizeof(severe_congestion));
    memset(moderate_congestion, 0, sizeof(severe_congestion));
    memset(severe_congestion, 0, sizeof(severe_congestion));
    memset(buffer_LCD_show, 0, sizeof(buffer_LCD_show));
    memset(temp, 0, sizeof(temp));
//    char free_flow[256] = "";
//    int all_zeros = 1; // 标志变量，假设所有值均为0
    float cal_congestion = 0;
    int No = 0;


    no_congestion_nodes_numbers = 0;
    mid_congestion_nodes_numbers = 0;
    moderate_congestion_nodes_numbers = 0;
    severe_congestion_nodes_numbers = 0;

    // 构建包含output_nodes值的字符串
    for (int i = 0; i < nodes_OUTPUTS; i++)
    {

//            if (i == nodes_OUTPUTS - 1)
//            {
//                sprintf(temp, "%d", i+1);
//            }
//            else
//            {
//                sprintf(temp, "%d,", i+1);
//            }
            sprintf(temp, "%d,", i+1);
            if (output_values[i] > 60.0)
            {
//                strcat(free_flow, temp);
                no_congestion_nodes_numbers++;
                continue;
            }

            else if (output_values[i] > 45.0)
            {
                strcat(mild_congestion, temp);
                mid_congestion_nodes_numbers++;
                continue;
            }
            else if (output_values[i] > 30.0)
            {
                strcat(moderate_congestion, temp);
                moderate_congestion_nodes_numbers++;
                continue;
            }
//            else if (output_values[i] <= 20.0)
            else
            {
                strcat(severe_congestion, temp);
                severe_congestion_nodes_numbers++;
                continue;
            }
        }
    cal_congestion = Cal_Congestion_Degree(no_congestion_nodes_numbers, mid_congestion_nodes_numbers, moderate_congestion_nodes_numbers, severe_congestion_nodes_numbers);
//    No = Cal_Congestion_Degree_NO(output_aver_speed);
    No = Cal_Congestion_Degree_NO(output_aver_speed);
    snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "In the next %d mins, ", prediction_time);
    lcd_show_string(0, 0, 16, buffer_LCD_show);
    if(strlen(mild_congestion) == 0 && strlen(moderate_congestion) == 0 && strlen(severe_congestion) == 0 )
    {
        lcd_show_string(0, 30, 16, "good traffic is expected");
    }

    if (strlen(mild_congestion) > 0)
    {
        snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "%s", mild_congestion);
        lcd_show_string(0, 16, 16, "Nodes with mild congestion");
        lcd_show_string(0, 32, 16, buffer_LCD_show);
        printf("Nodes with mild congestion: %s\n", mild_congestion);
    }

    if (strlen(moderate_congestion) > 0)
    {
        snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "%s", moderate_congestion);
        lcd_show_string(0, 80, 16, "Nodes with moderate congestion");
        lcd_show_string(0, 96, 16, buffer_LCD_show);
        printf("Nodes with moderate congestion: %s\n", moderate_congestion);
    }

    if (strlen(severe_congestion) > 0)
    {
        memset(buffer_LCD_show,0,sizeof(buffer_LCD_show));
        snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "%s", severe_congestion);
        lcd_show_string(0, 144, 16, "Nodes with severe congestion");
        lcd_show_string(0, 160, 16, buffer_LCD_show);
        printf("Nodes with severe congestion: %s\n", severe_congestion);
    }
    memset(buffer_LCD_show,0,sizeof(buffer_LCD_show));
    snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "Road congestion ratio:%.1f​​",cal_congestion);
    lcd_show_string(0, 200, 16, buffer_LCD_show);

    app_temp_humi_entry();
    memset(buffer_LCD_show,0,sizeof(buffer_LCD_show));
    snprintf(buffer_LCD_show, sizeof(buffer_LCD_show), "hum and temp:%.1f,%.1f​​",humidity,temperature);
    lcd_show_string(0, 220, 16, buffer_LCD_show);

    wavplay_enable(No);

//    printf("buffer: %s\n", buffer);

    return RT_EOK;
}









/**
 * The callback of network ready event
 */
void wlan_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_sem_release(&net_ready);
}

/**
 * The callback of wlan disconected event
 */
void wlan_station_disconnect_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    LOG_I("disconnect from the network!");
}

int store_float_array_from_json(const char *buffer,int group_number,int prediction_time) {

    int found_less_than_20 = 0;
    int found_less_equal_45 = 0;
    output_aver_speed = 0;
    output_total_speed = 0;

    cJSON *json = cJSON_Parse(buffer); // 解析JSON数据
    if (json == NULL) {
        printf("解析JSON出错：%s\n", cJSON_GetErrorPtr());//   wifi join RedmiK70 123456789
        return RT_EOK;
    }

    cJSON *outputs = cJSON_GetObjectItemCaseSensitive(json, "outputs"); // 获取名为"outputs"的数组
    if (!cJSON_IsArray(outputs)) {
        printf("错误：'outputs'不是数组。\n");
        cJSON_Delete(json);
        return RT_EOK;
    }

    int outputs_size = cJSON_GetArraySize(outputs); // 获取数组大小
    LOG_D("-----从JSON中提取的float数组：-----");
    LOG_D("Group %d---Pred %d mins", group_number, prediction_time);
    LOG_D("-----预测值-----");
    int n = 0;
    float FREEDOM_SPEED = 80;
    for (int i = 0; i < outputs_size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(outputs, i); // 获取数组中的元素
        if (cJSON_IsNumber(item))
        {
            output_values[i] = (float)item->valuedouble; // 转换为float类型

            if(i < 185)
            {
                output_total_speed += output_values[i];
            }
            if(output_values[i]<=45  && i <= 185)
            {
                output_nodes[n] = i + 1;
                output_speed[n] = output_values[i];
//                printf("%d, \n", output_nodes[n]);
//                printf("%d, \n", output_speed[n]);
                n = n + 1;
            }

            if(output_values[i]<=20  && i <= 185){
                found_less_than_20 = 1;
            }
            else if (output_values[i]<=45  && i <= 185){
                found_less_equal_45 = 1;
            }

            // 计算前 10 个数据的拥堵度并存储到 jams 中
            if (i < 10) {
                float congestion = ((FREEDOM_SPEED - output_values[i]) / FREEDOM_SPEED) * 100.0;
                jams[i] = (int)congestion;
            }

            if(i==184)
                printf("%.1f, \n-----真实值-----\n", output_values[i]);// 打印float数据
            else{
                printf("%.1f, ", output_values[i]);// 打印float数据
                fflush(stdout);  // 确保输出被刷新到控制台
            }
        }
    }
    // 打印拥堵度
    /*printf("Jams (congestion levels):\n");
    for (int i = 0; i < 10; i++) {
        printf("%d ", jams[i]);
    }*/
  output_aver_speed = output_total_speed/185;
  printf("预测速度的平均值为%.1f \n",output_aver_speed);

    if(found_less_than_20 == 1){
        rt_pin_write(PIN_LED_R, LED_ON);
        rt_pin_write(PIN_LED_B, LED_OFF);
    }
    else if (found_less_equal_45 == 1){
        rt_pin_write(PIN_LED_R, LED_ON);
        rt_pin_write(PIN_LED_B, LED_ON);
    }
    else{
        rt_pin_write(PIN_LED_R, LED_OFF);
        rt_pin_write(PIN_LED_B, LED_ON);
    }

    cJSON_Delete(json); // 清理cJSON对象

    return RT_EOK;
}

int webclient_post_file(const char* URI, const char* filename, const char* form_data)
{
    size_t length;
    char boundary[60];
    int fd = -1, rc = WEBCLIENT_OK;
    char *header = RT_NULL, *header_ptr;
    unsigned char *buffer = RT_NULL, *buffer_ptr;
    struct webclient_session* session = RT_NULL;
    int resp_data_len = 0;

    LOG_D("-----开始加载文件-----");

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
        LOG_E("post file failed, open file(%s) error.", filename);
        rc = -WEBCLIENT_FILE_ERROR;
        goto __exit;
    }

    /* 获取文件大小 */
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    //LOG_D("-----文件大小: %d字节-----", length);

    buffer = (unsigned char *) web_calloc(1, WEBCLIENT_RESPONSE_BUFSZ);
    if (buffer == RT_NULL)
    {
        LOG_E("post file failed, no memory for response buffer.");
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }

    header = (char *) web_calloc(1, WEBCLIENT_HEADER_BUFSZ);
    if (header == RT_NULL)
    {
        LOG_E("post file failed, no memory for header buffer.");
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }
    header_ptr = header;

    /* 构建边界 */
    rt_snprintf(boundary, sizeof(boundary), "----------------------------%012d", rt_tick_get());

    /* 构建封装的 mime_multipart 信息 */
    buffer_ptr = buffer;
    /* 第一个边界 */
    buffer_ptr += rt_snprintf((char*) buffer_ptr,
            WEBCLIENT_RESPONSE_BUFSZ - (buffer_ptr - buffer), "--%s\r\n", boundary);
    buffer_ptr += rt_snprintf((char*) buffer_ptr,
            WEBCLIENT_RESPONSE_BUFSZ - (buffer_ptr - buffer),
            "Content-Disposition: form-data; %s\r\n", form_data);
    buffer_ptr += rt_snprintf((char*) buffer_ptr,
            WEBCLIENT_RESPONSE_BUFSZ - (buffer_ptr - buffer),
            "Content-Type: application/octet-stream\r\n\r\n");    // 这里设置为 text/plain 以支持 TXT 文件   text/csv 以支持 csv 文件     application/octet-stream  以支持 npy / xlsx文件
    /* 计算内容长度 */
    length += buffer_ptr - buffer;
    length += strlen(boundary) + 8; /* 添加最后的边界 */

    /* 构建上传的头 */
    header_ptr += rt_snprintf(header_ptr,
            WEBCLIENT_HEADER_BUFSZ - (header_ptr - header),
            "Content-Length: %d\r\n", length);
    header_ptr += rt_snprintf(header_ptr,
            WEBCLIENT_HEADER_BUFSZ - (header_ptr - header),
            "Content-Type: multipart/form-data; boundary=%s\r\n", boundary);

    session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
    if(session == RT_NULL)
    {
        LOG_E("post file failed, create session error.");
        rc = -WEBCLIENT_NOMEM;
        goto __exit;
    }

    strncpy(session->header->buffer, header, strlen(header));
    session->header->length = strlen(session->header->buffer);

    rc = webclient_post(session, URI, NULL, 0);
    if(rc < 0)
    {
        LOG_E("post file failed, webclient_post error.");
        goto __exit;
    }

    LOG_D("-----发送预处理数据-----");

    /* 发送 mime_multipart */
    webclient_write(session, buffer, buffer_ptr - buffer);

    /* 发送文件数据 */
    while (1)
    {
        length = read(fd, buffer, WEBCLIENT_RESPONSE_BUFSZ);
        if (length <= 0)
        {
            break;
        }

        webclient_write(session, buffer, length);
    }

    /* 发送最后的边界 */
    rt_snprintf((char*) buffer, WEBCLIENT_RESPONSE_BUFSZ, "\r\n--%s--\r\n", boundary);
    webclient_write(session, buffer, strlen(boundary) + 8);

    extern int webclient_handle_response(struct webclient_session *session);
    if( webclient_handle_response(session) != 200)
    {
        LOG_E("post file failed, handle response error.");
        rc = -WEBCLIENT_ERROR;
        goto __exit;
    }

    resp_data_len = webclient_content_length_get(session);
    if (resp_data_len > 0)
    {
        int bytes_read = 0;

        rt_memset(buffer, 0x00, WEBCLIENT_RESPONSE_BUFSZ);
        do
        {
            bytes_read = webclient_read(session, buffer,
                resp_data_len < WEBCLIENT_RESPONSE_BUFSZ ? resp_data_len : WEBCLIENT_RESPONSE_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }
            resp_data_len -= bytes_read;
        } while(resp_data_len > 0);

        LOG_D("-----接收推理后数据-----");

        store_float_array_from_json((const char *)buffer,count,count_depths); // 存储JSON数据中的float数组

    }
__exit:
    if (fd >= 0)
    {
        close(fd);
    }

    if (session != RT_NULL)
    {
        webclient_close(session);
        session = RT_NULL;
    }

    if (buffer != RT_NULL)
    {
        web_free(buffer);
    }

    if (header != RT_NULL)
    {
        web_free(header);
    }

    return rc;//   wifi join RedmiK70 123456789
}

void inference_sub(void *args)
{
    if(count>1 && count_add0 == 1) {
        count--;
        count_add0 = 0;
    }
}

void inference_add(void *args)
{
    if(count<10 && count_add0 == 1){
        count++;
        count_add0 = 0;
    }
}

void inference_depths(void *args)
{
    if(count_depths<45 && count_add1 == 1) {
        count_add1 = 0;
        auto_count_add = auto_count_add ^ 1;
    }
}

void led_blue_turn(void *args)
{
    if(count_add2 == 1) {
        count_enlarged = count_enlarged ^ 1;
        count_add2 = 0;
    }
}

//void inference_depths(void *args)
//{
//    if(count_depths<45 && count_add == 1) {
//        count_depths = count_depths + 5;
//        count_add = 0;
//    }
//}

int thread_pin(void)
{

    /* 设置 RGB 灯引脚为输出模式 */
    rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_LED_R, LED_OFF);
    rt_pin_write(PIN_LED_B, LED_OFF);
    /*设置按键引脚为输入模式*/
    rt_pin_mode(KEY_UP_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_DOWN_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_LEFT_NUM, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_RIGHT_NUM, PIN_MODE_INPUT_PULLUP);
    /* 绑定中断，下降沿模式 */
    rt_pin_attach_irq(KEY_UP_NUM, PIN_IRQ_MODE_FALLING , led_blue_turn, RT_NULL);
    rt_pin_attach_irq(KEY_DOWN_NUM, PIN_IRQ_MODE_FALLING , inference_depths, RT_NULL);
    rt_pin_attach_irq(KEY_LEFT_NUM, PIN_IRQ_MODE_FALLING , inference_sub, RT_NULL);
    rt_pin_attach_irq(KEY_RIGHT_NUM, PIN_IRQ_MODE_FALLING , inference_add, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(KEY_UP_NUM, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_DOWN_NUM, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_LEFT_NUM, PIN_IRQ_ENABLE);
    rt_pin_irq_enable(KEY_RIGHT_NUM, PIN_IRQ_ENABLE);

    return RT_EOK;
}


static void example_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    // 定义一个指向 MQTT 主题信息结构的指针，并将 msg->msg 转换为 iotx_mqtt_topic_info_t 类型
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    // 根据事件类型进行处理
    switch (msg->event_type) {
        // 如果事件类型是接收到发布的消息
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* 打印主题名称和主题消息 */
            EXAMPLE_TRACE("Message Arrived:"); // 打印消息到达
            EXAMPLE_TRACE("Topic  : %.*s", topic_info->topic_len, topic_info->ptopic); // 打印主题名称
            EXAMPLE_TRACE("Payload: %.*s", topic_info->payload_len, topic_info->payload); // 打印消息内容
            EXAMPLE_TRACE("\n"); // 打印换行符
            break;
        default:
            break;
    }
}


static int example_subscribe(void *handle)
{
    int res = 0;
    //const char *fmt = "/%s/%s/user/get";  // 定义主题格式，包含两个占位符（产品密钥和设备名称）
    const char *fmt = "/sys/%s/%s/thing/event/property/post";
    char *topic = NULL;                  // 指向主题字符串的指针
    int topic_len = 0;                   // 主题字符串的长度

    // 计算主题字符串的长度
    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);       // 为主题字符串分配内存
    if (topic == NULL) {                 // 检查内存分配是否成功
        EXAMPLE_TRACE("memory not enough");  // 如果失败，打印错误信息
        return -1;                       // 返回错误码
    }
    memset(topic, 0, topic_len);         // 将分配的内存清零
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);  // 格式化主题字符串，将产品密钥和设备名称插入到主题格式中

    // 订阅主题
    res = IOT_MQTT_Subscribe(handle, topic, IOTX_MQTT_QOS0, example_message_arrive, NULL);
    if (res < 0) {                       // 检查订阅是否成功
        EXAMPLE_TRACE("subscribe failed");  // 如果失败，打印错误信息
        HAL_Free(topic);                 // 释放分配的内存
        return -1;                       // 返回错误码
    }

    HAL_Free(topic);                     // 订阅成功后，释放分配的内存
    return 0;                            // 返回成功码
}

void format_output(char *buffer, size_t buffer_size, float *output_values, int num_values) {
    // Initialize the buffer with the initial part of the JSON string
    snprintf(buffer, buffer_size, "{\"params\":{");

    // Append speed values to the buffer
    for (int i = 0; i < num_values; i++) {
        memset(temp_zhenshi,0,sizeof(temp_zhenshi));
        snprintf(temp_zhenshi, sizeof(temp_zhenshi), "\"speed%d\":%.1f", i + 1, output_values[i]);
        strncat(buffer, temp_zhenshi, buffer_size - strlen(buffer) - 1);

        if (i < num_values ) {
            strncat(buffer, ",", buffer_size - strlen(buffer) - 1);
        }
    }

    for (int k = 0; k < 10; k++) {
        memset(temp_zhenshi,0,sizeof(temp_zhenshi));
        snprintf(temp_zhenshi, sizeof(temp_zhenshi), "\"%d\":%d", k + 1, jams[k]);
        strncat(buffer, temp_zhenshi, buffer_size - strlen(buffer) - 1);

        if (k < 10 - 1) {
            strncat(buffer, ",", buffer_size - strlen(buffer) - 1);
        }
    }

    // Append pred_nodes arrays to the buffer
    float* pred_nodes[] = {pred_nodes1, pred_nodes2, pred_nodes3, pred_nodes4, pred_nodes5,
                           pred_nodes6, pred_nodes7, pred_nodes8, pred_nodes9, pred_nodes10};
    for (int j = 0; j < 10; j++) {
        // Add the start of the array
        memset(temp_zhenshi,0,sizeof(temp_zhenshi));
        snprintf(temp_zhenshi, sizeof(temp_zhenshi), ",\"s%d\":[", j + 1);
        strncat(buffer, temp_zhenshi, buffer_size - strlen(buffer) - 1);

        // Append each element in the array
        for (int i = 0; i < pred_OUTPUTS; i++) {
            snprintf(temp_zhenshi, sizeof(temp_zhenshi), "%.1f", pred_nodes[j][i]);
            strncat(buffer, temp_zhenshi, buffer_size - strlen(buffer) - 1);

            // Append a comma if it's not the last element
            if (i < pred_OUTPUTS - 1) {
                strncat(buffer, ",", buffer_size - strlen(buffer) - 1);
            }
        }

        // Close the array
        strncat(buffer, "]", buffer_size - strlen(buffer) - 1);
    }

    // Close the JSON string
    strncat(buffer, "}}", buffer_size - strlen(buffer) - 1);
}
//{"data":[{"time":"5min","PRED":0},{"time":"10min","PRED":0},{"time":"15min","PRED":0},{"time":"20min","PRED":0},{"time":"25min","PRED":0},{"time":"30min","PRED":0},{"time":"35min","PRED":0},{"time":"40min","PRED":0},{"time":"45min","PRED":0}]}
static int example_publish(void *handle)
{
    int             res = 0;                                    // 初始化结果变量 res 为 0
    //const char     *fmt = "/%s/%s/user/get";                    // 定义主题格式字符串 fmt   /sys/${ProductKey}/${DeviceName}/thing/event/property/post
    const char     *fmt = "/sys/%s/%s/thing/event/property/post";
    char           *topic = NULL;                               // 初始化主题字符串指针 topic 为 NULL
    int             topic_len = 0;                              // 初始化主题字符串长度 topic_len 为 0
    //char           *payload = "{\"message\":\"hello!\"}";       // 定义消息负载字符串 payload

//    char buffer_publish[2048];
    memset(buffer_publish,0,sizeof(buffer_publish));
    format_output(buffer_publish, sizeof(buffer_publish), output_values, 10);
    //printf("%s\n", buffer);
    // snprintf(buffer,sizeof(buffer),"{\"params\":{\"message\":%f}}",temperature);
    // 计算主题字符串的长度
    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);                              // 为主题字符串分配内存
    if (topic == NULL) {                                        // 检查内存分配是否成功
        EXAMPLE_TRACE("memory not enough");                     // 如果失败，打印错误信息
        return -1;                                              // 返回错误码//   wifi join RedmiK70 123456789    ali_mqtt_sample
    }
    memset(topic, 0, topic_len);                                // 将分配的内存清零
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);  // 格式化主题字符串，将产品密钥和设备名称插入到主题格式中

    // 发布消息
    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, buffer_publish, strlen(buffer_publish));
    //LOG_D("Publish result: %d", res);                           // 打印 res 的值
    if (res < 0) {                                              // 检查发布是否成功
        EXAMPLE_TRACE("publish failed, res = %d", res);         // 如果失败，打印错误信息
        HAL_Free(topic);                                        // 释放分配的内存
        return -1;                                              // 返回错误码
    }

    HAL_Free(topic);                                            // 发布成功后，释放分配的内存
    return 0;                                                   // 返回成功码
}

static int example_publish1(void *handle)
{
    int             res = 0;                                    // 初始化结果变量 res 为 0
    //const char     *fmt = "/%s/%s/user/get";                    // 定义主题格式字符串 fmt   /sys/${ProductKey}/${DeviceName}/thing/event/property/post
    const char     *fmt = "/sys/%s/%s/thing/event/property/post";
    char           *topic = NULL;                               // 初始化主题字符串指针 topic 为 NULL
    int             topic_len = 0;                              // 初始化主题字符串长度 topic_len 为 0
    //char           *payload = "{\"message\":\"hello!\"}";       // 定义消息负载字符串 payload

    memset(buffer_publish1,0,sizeof(buffer_publish1));
    snprintf(buffer_publish1,sizeof(buffer_publish1),"{\"params\":{\"z1\":%.1f,\"z2\":%.1f,\"z3\":%.1f,\"z4\":%.1f,\"z5\":%.1f,\"z6\":%.1f,\"z7\":%.1f,\"z8\":%.1f,\"z9\":%.1f,\"z10\":%.1f}}",
            zhenshi[line][0],zhenshi[line][1],zhenshi[line][2],zhenshi[line][3],zhenshi[line][4],zhenshi[line][5],zhenshi[line][6],zhenshi[line][7],zhenshi[line][8],zhenshi[line][9]);
    // 计算主题字符串的长度
    topic_len = strlen(fmt) + strlen(DEMO_PRODUCT_KEY) + strlen(DEMO_DEVICE_NAME) + 1;
    topic = HAL_Malloc(topic_len);                              // 为主题字符串分配内存
    if (topic == NULL) {                                        // 检查内存分配是否成功
        EXAMPLE_TRACE("memory not enough");                     // 如果失败，打印错误信息
        return -1;                                              // 返回错误码//   wifi join RedmiK70 123456789    ali_mqtt_sample
    }
    memset(topic, 0, topic_len);                                // 将分配的内存清零
    HAL_Snprintf(topic, topic_len, fmt, DEMO_PRODUCT_KEY, DEMO_DEVICE_NAME);  // 格式化主题字符串，将产品密钥和设备名称插入到主题格式中

    // 发布消息
    res = IOT_MQTT_Publish_Simple(0, topic, IOTX_MQTT_QOS0, buffer_publish1, strlen(buffer_publish1));
    //LOG_D("Publish result: %d", res);                           // 打印 res 的值
    if (res < 0) {                                              // 检查发布是否成功
        EXAMPLE_TRACE("publish failed, res = %d", res);         // 如果失败，打印错误信息
        HAL_Free(topic);                                        // 释放分配的内存
        return -1;                                              // 返回错误码
    }

    HAL_Free(topic);                                            // 发布成功后，释放分配的内存
    return 0;                                                   // 返回成功码
}

static void example_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    EXAMPLE_TRACE("msg->event_type : %d", msg->event_type);
}

static int mqtt_example_main(int argc, char *argv[])
{
    void *pclient = NULL;  // 初始化 MQTT 客户端指针 pclient 为 NULL
    int res = 0;  // 初始化结果变量 res 为 0
    iotx_mqtt_param_t mqtt_params;  // 声明 MQTT 参数结构体 mqtt_params

    time_t last_publish1_time = time(RT_NULL); // 记录上次 example_publish1 的时间
    int publish_done = 0; // 用于标记是否执行了 example_publish

    // 从硬件抽象层获取产品密钥、设备名称和设备密钥
    HAL_GetProductKey(DEMO_PRODUCT_KEY);
    HAL_GetDeviceName(DEMO_DEVICE_NAME);
    HAL_GetDeviceSecret(DEMO_DEVICE_SECRET);

    EXAMPLE_TRACE("mqtt example");  // 打印调试信息，表明 MQTT 示例开始运行

    // 初始化 MQTT 参数结构体
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));  // 将 mqtt_params 结构体内存清零

    mqtt_params.write_buf_size = 8192;
    mqtt_params.read_buf_size = 8192;

    mqtt_params.handle_event.h_fp = example_event_handle;  // 设置事件处理函数

    // 构造 MQTT 客户端
    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient) {  // 检查 MQTT 客户端是否成功构造
        EXAMPLE_TRACE("MQTT construct failed");  // 打印错误信息
        return -1;  // 返回错误码 -1
    }

    // 订阅 MQTT 主题
    res = example_subscribe(pclient);
    if (res < 0) {  // 检查订阅是否成功
        IOT_MQTT_Destroy(&pclient);  // 如果失败，销毁 MQTT 客户端
        return -1;  // 返回错误码 -1
    }

    rt_system_timer_init();

    rt_system_timer_thread_init();

    wavplay_init();

    wavplay_enable(0);



    while (1) {  // 进入主循环

        if (count != last_count)
            {
                // 使用 memset 将所有元素设置为0
                memset(output_nodes, 0, sizeof(output_nodes));
                count_add0 = 1;
                rt_pin_write(PIN_LED_R, LED_OFF);
                rt_pin_write(PIN_LED_B, LED_OFF);
                process_count(count);
                last_count = count; // 更新 last_count 为当前的 count 值
                count_depths = 5;
                pred_depths = 1;
                count_enlarged = 0;
                last_count_enlarged = 0;
                assign_values(pred_depths, output_values, pred_nodes1, pred_nodes2, pred_nodes3, pred_nodes4, pred_nodes5, pred_nodes6, pred_nodes7, pred_nodes8, pred_nodes9, pred_nodes10);
                example_publish(pclient);  // 调用发布函数
                rt_thread_mdelay(1000);
                // 调用 IOT_MQTT_Yield 并打印返回值
                IOT_MQTT_Yield(pclient, 1000);
                count_add1 = 1;
                count_add0 = 1;
                publish_done = 0;
            }

//        if (count_depths != last_count_depths)
//            {
//                // 使用 memset 将所有元素设置为0
//                memset(output_nodes, 0, sizeof(output_nodes));
//                process_count_depths(count_depths);
//                last_count_depths = count_depths;
//                count_enlarged = 0;
//                count_add = 1;
//            }

        if (count_depths<45 && auto_count_add == 1)//   wifi join RedmiK70 123456789    ali_mqtt_sample
            {
                // 使用 memset 将所有元素设置为0
                LOG_D("-----自动推理-----");
                rt_pin_write(PIN_LED_R, LED_OFF);
                rt_pin_write(PIN_LED_B, LED_OFF);
                count_depths = count_depths + 5;
                pred_depths = pred_depths + 1;
                memset(output_nodes, 0, sizeof(output_nodes));//     ali_mqtt_sample
                count_add1 = 1;
                process_count_depths(count_depths);
                assign_values(pred_depths, output_values, pred_nodes1, pred_nodes2, pred_nodes3, pred_nodes4, pred_nodes5, pred_nodes6, pred_nodes7, pred_nodes8, pred_nodes9, pred_nodes10);
                count_enlarged = 0;
                last_count_enlarged = 0;
                example_publish(pclient);  // 调用发布函数
                publish_done = 1; // 设置标记为1，表示已执行 example_publish
                if(count_depths == 45){
                    auto_count_add = auto_count_add ^ 1;
                    publish_done = 0;
                }
                rt_thread_mdelay(2000);
                // 调用 IOT_MQTT_Yield 并打印返回值
                IOT_MQTT_Yield(pclient, 1000);
                count_add1 = 1;
            }

        if (count_enlarged != last_count_enlarged)
            {
                process_count_enlarged(count_enlarged);
                last_count_enlarged = count_enlarged;
                rt_thread_mdelay(500);
                count_add1 = 1;
                count_add2 = 1;
            }

        //IOT_MQTT_Yield(pclient, 200);  // 等待 200 毫秒，处理 MQTT 消息和事件

        // 每分钟调用一次 example_publish1
        time_t current_time = time(RT_NULL);
        if (difftime(current_time, last_publish1_time) >= 60 && !publish_done) {
            example_publish1(pclient); // 调用发布函数
            last_publish1_time = current_time; // 更新上次调用时间
            rt_thread_mdelay(1000);
            // 更新当前行号
            line++;
            if (line >= total_lines) {
                line = 0;  // 重置行号以便循环
            }
            // 调用 IOT_MQTT_Yield 并打印返回值
            IOT_MQTT_Yield(pclient, 1000);
        }
    }

    return 0;  // 返回成功码 0
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT_ALIAS(mqtt_example_main, ali_mqtt_sample, ali coap sample);
MSH_CMD_EXPORT(webnet_start, wenbet test);
#endif
