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
#include <psp2/kernel/clib.h>
#include <psp2/power.h>
#include <taihen.h>

static SceUID g_hooks[2];

#if 0
static tai_hook_ref_t ref_hook;
// offset 0x1844f0
static int status_draw_battery_patched(int a1, uint8_t a2)
{
    return TAI_CONTINUE(int, ref_hook, a1, a2);
}
#endif

static int in_draw_time = 0;

static tai_hook_ref_t ref_hook0;
static int status_draw_time_patched(int a1, int a2)
{
    in_draw_time = 1;
    int out = TAI_CONTINUE(int, ref_hook0, a1, a2);
    in_draw_time = 0;
    return out;
}

static tai_hook_ref_t ref_hook1;
static uint16_t **some_strdup_patched(uint16_t **a1, uint16_t *a2, int a2_size)
{
    if (in_draw_time) {
        int percent = scePowerGetBatteryLifePercent();
        if (percent >= 0 && percent <= 100) {
            char buff[10];
            int len = sceClibSnprintf(buff, 10, "  %d%%  ", percent);
            for (int i = 0; i < len; ++i) {
                a2[a2_size + i] = buff[i];
            }
            a2[a2_size + len] = 0;
            return TAI_CONTINUE(uint16_t**, ref_hook1, a1, a2, a2_size + len);
        }
    }
    return TAI_CONTINUE(uint16_t**, ref_hook1, a1, a2, a2_size);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
    LOG("Starting module");

    tai_module_info_t info;
    info.size = sizeof(info);
    if (taiGetModuleInfo("SceShell", &info) >= 0) {
        switch (info.module_nid) {
        case 0x0552F692: { // retail 3.60 SceShell
            g_hooks[0] = taiHookFunctionOffset(&ref_hook0,
                                               info.modid,
                                               0,         // segidx
                                               0x183ea4,  // offset
                                               1,         // thumb
                                               status_draw_time_patched);
            g_hooks[1] = taiHookFunctionOffset(&ref_hook1,
                                               info.modid,
                                               0,         // segidx
                                               0x40e0b4,  // offset
                                               1,         // thumb
                                               some_strdup_patched);
            break;
        }
        default:
            LOG("SceShell %XNID not recognized", info.module_nid);
            break;
        }
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    LOG("Stopping module");

    if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], ref_hook0);
    if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], ref_hook1);

    return SCE_KERNEL_STOP_SUCCESS;
}
