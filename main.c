/* Copyright (C) 2017 Yifan Lu
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <taihen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usb_detect.h"
#include "debugScreen.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define DISPLAY_WIDTH           960
#define DISPLAY_HEIGHT          544
#define DISPLAY_STRIDE_IN_PIXELS    1024
#define DISPLAY_BUFFER_COUNT        2
#define DISPLAY_MAX_PENDING_SWAPS   1

typedef struct{
    void*data;
    SceGxmSyncObject*sync;
    SceGxmColorSurface surf;
    SceUID uid;
}displayBuffer;

unsigned int backBufferIndex = 0;
unsigned int frontBufferIndex = 0;
/* could be converted as struct displayBuffer[] */
displayBuffer dbuf[DISPLAY_BUFFER_COUNT];

void *dram_alloc(unsigned int size, SceUID *uid){
    void *mem;
    *uid = sceKernelAllocMemBlock("gpu_mem", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(size,256*1024), NULL);
    sceKernelGetMemBlockBase(*uid, &mem);
    sceGxmMapMemory(mem, ALIGN(size,256*1024), SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
    return mem;
}
void gxm_vsync_cb(const void *callback_data){
    sceDisplaySetFrameBuf(&(SceDisplayFrameBuf){sizeof(SceDisplayFrameBuf),
        *((void **)callback_data),DISPLAY_STRIDE_IN_PIXELS, 0,
        DISPLAY_WIDTH,DISPLAY_HEIGHT}, SCE_DISPLAY_SETBUF_NEXTFRAME);
}
void gxm_init(){
    sceGxmInitialize(&(SceGxmInitializeParams){0,DISPLAY_MAX_PENDING_SWAPS,gxm_vsync_cb,sizeof(void *),SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE});
    unsigned int i;
    for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        dbuf[i].data = dram_alloc(4*DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, &dbuf[i].uid);
        sceGxmColorSurfaceInit(&dbuf[i].surf,SCE_GXM_COLOR_FORMAT_A8B8G8R8,SCE_GXM_COLOR_SURFACE_LINEAR,SCE_GXM_COLOR_SURFACE_SCALE_NONE,SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,DISPLAY_WIDTH,DISPLAY_HEIGHT,DISPLAY_STRIDE_IN_PIXELS,dbuf[i].data);
        sceGxmSyncObjectCreate(&dbuf[i].sync);
    }
}
void gxm_swap(){
    sceGxmPadHeartbeat(&dbuf[backBufferIndex].surf, dbuf[backBufferIndex].sync);
    sceGxmDisplayQueueAddEntry(dbuf[frontBufferIndex].sync, dbuf[backBufferIndex].sync, &dbuf[backBufferIndex].data);
    frontBufferIndex = backBufferIndex;
    backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
}
void gxm_term(){
    for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
        sceGxmUnmapMemory(dbuf[i].data);
        sceKernelFreeMemBlock(dbuf[i].uid);
    }
    sceGxmTerminate();
}

int main(int argc, char *argv[]) {
    char buf[LOG_BUF_SIZE];
    args_t arg;
    int ret;
    int uid1, uid2, uid3;
    int res;

    psvDebugScreenInit();

    psvDebugScreenPrintf("Started!\n\n");
    psvDebugScreenPrintf("Press X to start probing.\nPress Circle to stop probing and exit.\n\n");

    uid1 = -1, uid2 = -1, uid3 = -1;
    int i = 0;
    while (1) {

        SceCtrlData ctrl;
        sceCtrlPeekBufferPositive(0, &ctrl, 1);

        if (ctrl.buttons & SCE_CTRL_CIRCLE) {
            ret = sceKernelStopUnloadModule(uid2, 0, NULL, 0, NULL, &res);
            psvDebugScreenPrintf("User stop: %x, %x\n", ret, res);
            ret = taiStopUnloadKernelModule(uid1, 0, NULL, 0, NULL, &res);
            psvDebugScreenPrintf("Kernel stop: %x, %x\n", ret, res);
            ret = taiStopUnloadKernelModule(uid3, 0, NULL, 0, NULL, &res);
            psvDebugScreenPrintf("Kernel 2 stop: %x, %x\n", ret, res);

            sceKernelDelayThread(1*1000*1000);
            break;
        } else if (ctrl.buttons & SCE_CTRL_CROSS) {
            uid3 = taiLoadKernelModule("ux0:app/USBDETECT/kernel2.skprx", 0, NULL);
            if (uid3 < 0) {
                psvDebugScreenPrintf("Kernel load: %x\n", uid3);
            } else {
                ret = taiStartKernelModule(uid3, sizeof(arg), &arg, 0, NULL, &res);
                psvDebugScreenPrintf("Kernel start: %x, %x\n", ret, res);
            }

            uid1 = taiLoadKernelModule("ux0:app/USBDETECT/kernel.skprx", 0, NULL);
            if (uid1 < 0) {
                psvDebugScreenPrintf("Kernel load: %x\n", uid1);
            } else {
                ret = taiStartKernelModule(uid1, sizeof(arg), &arg, 0, NULL, &res);
                psvDebugScreenPrintf("Kernel start: %x, %x\n", ret, res);
            }

            uid2 = sceKernelLoadStartModule("ux0:app/USBDETECT/user.suprx", 0, NULL, 0, NULL, &res);
            psvDebugScreenPrintf("User load & start: %x, %x\n", uid2, res);

            sceKernelDelayThread(1*1000*1000);
        }

        if ((i++ % 1000) == 0 && uid2 > 0 && dump_trace(buf) > 0) {
            psvDebugScreenPuts(buf);
            sceKernelDelayThread(1*1000*1000);
        }
    }

    return 0;
}
