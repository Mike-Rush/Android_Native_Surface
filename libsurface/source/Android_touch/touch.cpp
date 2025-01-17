
// 原始触摸，不能解决触摸穿透问题
/*
#include "touch.h"


#include <android/input.h>
#include <android/keycodes.h>
#include <cstring>
#include <cstdio>
#include <malloc.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sys/system_properties.h>
#include <linux/input.h>
#include <linux/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include "imgui.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <queue>
#include <random>
#include <string>

// User libs
#include "draw.h"

#define UNGRAB 0x0
#define GRAB 0x1
#define BITS_PER_LONG (sizeof(long) * 8)
#define test_bit(array, bit)    ((array[bit / BITS_PER_LONG] >> bit % BITS_PER_LONG) & 1)
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)


#define FROM_SCREEN 0x0
#define FROM_SOCKET 0x1

#define UNGRAB 0x0
#define GRAB 0x1

#define UP 0x0
#define DOWN 0x1

using namespace std;

struct Vector2 {
    Vector2(float x, float y) {
        this->x = x;
        this->y = y;
    }

    Vector2() {}

    float x;
    float y;

    bool operator==(const Vector2 &t) const {
        if (this->x == t.x && this->y == t.y) return true;
        return false;
    }

    bool operator!=(const Vector2 &t) const {
        if (this->x != t.x || this->y != t.y) return true;
        return false;
    }
};


int isa_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

std::string getTouchScreenDevice() {
    struct dirent **namelist;
    int i, ndev;
    ndev = scandir("/dev/input", &namelist, isa_event_device, alphasort);
    if (ndev <= 0) {
        return "";
    }
    for (i = 0; i < ndev; i++) {
        char fname[64];
        int fd = -1;
        unsigned long keybit[NBITS(KEY_CNT)];
        unsigned long propbit[INPUT_PROP_MAX];
        snprintf(fname, sizeof(fname), "%s/%s", "/dev/input", namelist[i]->d_name);
        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            continue;
        }
        memset(keybit, 0, sizeof(keybit));
        memset(propbit, 0, sizeof(propbit));
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit);
        ioctl(fd, EVIOCGPROP(INPUT_PROP_MAX), propbit);
        close(fd);
        free(namelist[i]);
        if (test_bit(propbit, INPUT_PROP_DIRECT) &&
            (test_bit(keybit, BTN_TOUCH) || test_bit(keybit, BTN_TOOL_FINGER))) {
            return {fname};
        } else if (test_bit(keybit, BTN_TOUCH) || test_bit(keybit, BTN_TOOL_FINGER)) {
            return {fname};
        }
    }
    return "";
}


ImVec2 rotatePointx(uint32_t orientation, ImVec2 mxy, ImVec2 wh) {
    if (orientation == 0) {
        return mxy;
    }
    ImVec2 xy(mxy.x, mxy.y);
    if (orientation == 3) {
        xy.x = (float) wh.y - mxy.y;
        xy.y = mxy.x;
    } else if (orientation == 2) {
        xy.x = (float) wh.x - mxy.x;
        xy.y = (float) wh.y - mxy.y;
    } else if (orientation == 1) {
        xy.x = mxy.y;
        xy.y = (float) wh.x - mxy.x;
    }
    return xy;
}

ImVec2 getTouchScreenDimension(int fd) {
    int abs_x[6], abs_y[6] = {0};
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), abs_x);
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), abs_y);
    return {(float) abs_x[2], (float) abs_y[2]};
}

MDisplayInfo getTouchDisplyInfo() {
    if (displayInfo.orientation == 0 || displayInfo.orientation == 2) {
        return displayInfo;
    } else {
        return {displayInfo.height, displayInfo.width, displayInfo.orientation};
    }
}

bool touchFlag = false;

void touch_config() {
    std::string device = getTouchScreenDevice();
    // printf("touch event : %s\n",device.c_str());
    if (device.length() < 2) {
        printf("No Touch Event\n");
    }
    int touch_device_fd = open(device.c_str(), O_RDWR | O_SYNC | O_NONBLOCK);
    //打开设备驱动写入
    if (touch_device_fd < 0) {
        printf("Open dev Error\n");
    }
    // 屏蔽触摸
//    ioctl(touch_device_fd, EVIOCGRAB, GRAB);
    // 触摸事件队列
    vector<input_event> events;
    // 手指编号
    int fingerIndex = 0;
    // 触摸位置
    int eventX = 0, eventY = 0;
    input_event event{};
    ImVec2 touch_screen_size = getTouchScreenDimension(touch_device_fd);
    while (touchFlag) {
        if (read(touch_device_fd, &event, sizeof(event)) > 0) {
            if (event.type == EV_SYN && event.code == SYN_REPORT && event.value == 0) {
                int status = IM_MOVE;
                for (input_event e: events) {
                    switch (e.type) {
                        case EV_KEY: {
                            if (e.code == BTN_TOUCH) {
                                if (e.value == DOWN) {
                                    status = IM_DOWN;
                                } else if (e.value == UP) {
                                    status = IM_UP;
                                    break;
                                }
                            }
                            break;
                        }
                        case EV_ABS: {
                            if (e.code == ABS_MT_TRACKING_ID) {
                                if (e.value != -1) {
                                    status = IM_DOWN;
                                } else {
                                    status = IM_UP;
                                    break;
                                }
                            } else if (e.code == ABS_MT_SLOT) {
                                fingerIndex = e.value;
                            } else if (fingerIndex == 0) {
                                if (e.code == ABS_MT_POSITION_X) {
                                    eventX = e.value;
                                } else if (e.code == ABS_MT_POSITION_Y) {
                                    eventY = e.value;
                                }
                            }
                        }
                    }
                }

                events.clear();

                if (fingerIndex != 0) {
                    continue;
                }

                MDisplayInfo mDisplayInfo = getTouchDisplyInfo();
                ImVec2 point = rotatePointx(mDisplayInfo.orientation, {(float) eventX, (float) eventY},
                                            touch_screen_size);
                ImVec2 newEvent((point.x * (float) mDisplayInfo.width) / touch_screen_size.x,
                                (point.y * (float) mDisplayInfo.height) / touch_screen_size.y);
                ImGuInputEvent imGuInputEvent{};
                imGuInputEvent.fingerIndex = fingerIndex;
                imGuInputEvent.pos = newEvent;

                if (status == IM_DOWN) {           // 按下
                    imGuInputEvent.type = IM_DOWN;
                } else if (status == IM_MOVE) {    // 移动
                    imGuInputEvent.type = IM_MOVE;
                } else {                           // 放开
                    imGuInputEvent.type = IM_UP;
                }
                ImGui_ImplAndroid_HandleInputEvent(imGuInputEvent);
            } else {
                events.push_back(event);
            }
        }
        std::this_thread::sleep_for(0.0001s);
    }
    close(touch_device_fd);
    pthread_exit(0);
}

void initTouch() { // 初始化触摸设置
    touchFlag = true;
    std::thread touch_thread(touch_config);
    touch_thread.detach();
}

void closeTouch() {
    touchFlag = false;
}
*/
