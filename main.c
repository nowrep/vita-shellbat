/*
 *  ShellBat plugin
 *  Copyright (c) 2017 David Rosca
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */
#include "log.h"

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/net/netctl.h>
#include <psp2/power.h>
#include <taihen.h>

/*
offset 0x1844f0:
int status_draw_battery_patched(int a1, uint8_t a2);
*/

static SceUID g_hooks[2];

uint32_t text_addr, text_size, data_addr, data_size;

// Functions

/**
 * ScePafWidget_39B15B98
 *
 * @param widget  Widget ptr
 * @param size    Font size (22.0 - standard font size in statusbar)
 * @param unk0    Unknown, pass 1
 * @param pos     Start of part of text to set size
 * @param len     Length of part of text to set size from pos
 */
static int (*scePafWidgetSetFontSize)(void *widget, float size, int unk0, int pos, int len);

static void get_functions_retail_355()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x45c950;
}

static void get_functions_retail_360()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x45ce80;
}

static void get_functions_retail_365_368()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x45D2C8;
}

static void get_functions_testkit_360()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x453038;
}

static void get_functions_testkit_365()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x453478;
}

static void get_functions_devkit_360()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x44E5F8;
}

static void get_functions_devkit_365()
{
    scePafWidgetSetFontSize = (void*) text_addr + 0x44EA68;
}

static int digit_len(int num)
{
    if (num < 10) {
        return 1;
    } else if (num < 100) {
        return 2;
    } else {
        return 3;
    }
}

static int in_draw_time = 0;
static int ip_start = 0;
static int ip_len = 0;
static int ampm_start = -1;
static int bat_num_start = 0;
static int bat_num_len = 0;
static int percent_start = 0;

static tai_hook_ref_t ref_hook0;
static int status_draw_time_patched(void *a1, int a2)
{
    in_draw_time = 1;
    int out = TAI_CONTINUE(int, ref_hook0, a1, a2);
    in_draw_time = 0;
    if (a1) {
        // Restore AM/PM size
        if (ampm_start != -1) {
            scePafWidgetSetFontSize(a1, 16.0, 1, ampm_start, 2);
        }
        scePafWidgetSetFontSize(a1, 20.0, 1, bat_num_start, bat_num_len);
        scePafWidgetSetFontSize(a1, 16.0, 1, percent_start, 1);

        if (ip_len > 0) {
            scePafWidgetSetFontSize(a1, 20.0, 1, ip_start, ip_len);
        }
    }
    return out;
}

static tai_hook_ref_t ref_hook1;
static uint16_t **some_strdup_patched(uint16_t **a1, uint16_t *a2, int a2_size)
{
    if (in_draw_time) {
        if (a2[a2_size - 1] == 'M') {
            ampm_start = a2_size - 2;
        } else {
            ampm_start = -1;
        }

        int position = a2_size;

        a2[position++] = ' ';
        a2[position++] = ' ';

        SceNetCtlInfo info = {0};
        if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) >= 0) {
            ip_start = position;

            int i;
            for (i = 0; i < 16 && info.ip_address[i] != 0; ++i) {
                a2[position + i] = info.ip_address[i];
            }
            
            position += + i;
            ip_len = i;

            a2[position++] = ' ';
            a2[position++] = ' ';
        }

        static int oldpercent = 0;
        int percent = scePowerGetBatteryLifePercent();
        if (percent < 0 || percent > 100) {
            percent = oldpercent;
        }
        oldpercent = percent;
        char buff[10];
        int perc_len = sceClibSnprintf(buff, 10, "%d%%", percent);
        for (int i = 0; i < perc_len; ++i) {
            a2[position + i] = buff[i];
        }
        bat_num_start = position;
        bat_num_len = digit_len(percent);
        percent_start = bat_num_start + bat_num_len;

        position += perc_len;
        a2[position] = 0;
        a2_size = position;

        return TAI_CONTINUE(uint16_t**, ref_hook1, a1, a2, a2_size);
    }
    return TAI_CONTINUE(uint16_t**, ref_hook1, a1, a2, a2_size);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
    LOG("Starting module");

    tai_module_info_t info;
    info.size = sizeof(info);
    int ret = taiGetModuleInfo("SceShell", &info);
    if (ret < 0) {
        LOG("taiGetModuleInfo error %X", ret);
        return SCE_KERNEL_START_FAILED;
    }

    // Modified from TheOfficialFloW/Adrenaline
    SceKernelModuleInfo mod_info;
    mod_info.size = sizeof(SceKernelModuleInfo);
    ret = sceKernelGetModuleInfo(info.modid, &mod_info);
    if (ret < 0) {
        LOG("sceKernelGetModuleInfo error %X", ret);
        return SCE_KERNEL_START_FAILED;
    }
    text_addr = (uint32_t) mod_info.segments[0].vaddr;
    text_size = (uint32_t) mod_info.segments[0].memsz;
    data_addr = (uint32_t) mod_info.segments[1].vaddr;
    data_size = (uint32_t) mod_info.segments[1].memsz;

    uint32_t offsets[2];

    switch (info.module_nid) {
    case 0x8978D25D: // retail 3.55 SceShell
        offsets[0] = 0x183EF0;
        offsets[1] = 0x40DBD8;
        get_functions_retail_355();
        break;

    case 0x0552F692: // retail 3.60 SceShell
        offsets[0] = 0x183ea4;
        offsets[1] = 0x40e0b4;
        get_functions_retail_360();
        break;

    case 0x5549BF1F: // retail 3.65 SceShell
    case 0x34B4D82E: // retail 3.67 SceShell
    case 0x12DAC0F3: // retail 3.68 SceShell
        offsets[0] = 0x183F6C;
        offsets[1] = 0x40E4FC;
        get_functions_retail_365_368();
        break;

    case 0xEAB89D5C: // PTEL 3.60 SceShell
        offsets[0] = 0x17C2D8;
        offsets[1] = 0x404828;
        get_functions_testkit_360();
        break;

    case 0x587F9CED: // PTEL 3.65 SceShell
        offsets[0] = 0x17C3A0;
        offsets[1] = 0x404C68;
        get_functions_testkit_365();
        break;

    case 0x6CB01295: // PDEL 3.60 SceShell
        offsets[0] = 0x17B8DC;
        offsets[1] = 0x400028;
        get_functions_devkit_360();
        break;

    case 0xE6A02F2B: // PDEL 3.65 SceShell
        offsets[0] = 0x17B9A4;
        offsets[1] = 0x400498;
        get_functions_devkit_365();
        break;

    default:
        LOG("SceShell %X NID not recognized", info.module_nid);
        return SCE_KERNEL_START_FAILED;
    }

    g_hooks[0] = taiHookFunctionOffset(&ref_hook0,
                                       info.modid,
                                       0,          // segidx
                                       offsets[0], // offset
                                       1,          // thumb
                                       status_draw_time_patched);
    g_hooks[1] = taiHookFunctionOffset(&ref_hook1,
                                       info.modid,
                                       0,          // segidx
                                       offsets[1], // offset
                                       1,          // thumb
                                       some_strdup_patched);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    LOG("Stopping module");

    if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], ref_hook0);
    if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], ref_hook1);

    return SCE_KERNEL_STOP_SUCCESS;
}
