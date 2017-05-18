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

#include <psp2/io/fcntl.h>

void _log(char *str, ...)
{
    static SceUID logfd = -1;
    if (logfd == -1) {
        logfd = sceIoOpen("ux0:shellbat.log", SCE_O_APPEND | SCE_O_CREAT | SCE_O_WRONLY, 0666);
    }
    char buff[256];
    va_list arglist;
    va_start(arglist, str);
    int len = sceClibVsnprintf(buff, 256, str, arglist);
    va_end(arglist);
    sceIoWrite(logfd, buff, len);
    sceIoWrite(logfd, "\n", 1);
}
