/* Copyright (C) 2017 Yifan Lu
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <psp2/kernel/modulemgr.h>
#include "usb_detect.h"

int dump_trace(char *buf) {
    return k_dump_trace(buf);
}

int module_start(int args, void *argv) {
    (void)args;
    (void)argv;
    return SCE_KERNEL_START_SUCCESS;
}
void _start() __attribute__ ((weak, alias ("module_start")));

int module_stop() {
    return SCE_KERNEL_STOP_SUCCESS;
}
