/* Copyright (C) 2017 Yifan Lu
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/usbd.h>
#include "usb_detect.h"
#include <taihen.h>

int printf(const char *fmt, ...);
int snprintf(char *buf, int len, const char *fmt, ...);

volatile static int lock = 0;
volatile static char buf[LOG_BUF_SIZE] = {0};
volatile static int at = 0;

#define TRACEF(fmt, args...) do { \
  int len; \
  printf(fmt, args); \
  while (lock); \
  lock = 1; \
  len = snprintf((char *)buf+at, LOG_BUF_SIZE-at, fmt, args); \
  if (at + len < LOG_BUF_SIZE) \
    at += len; \
  lock = 0; \
} while (0)

int usb_probe(int device_id);
int usb_attach(int device_id);
int usb_detach(int device_id);

static SceUsbdDriver driver = {
  .name = "detect",
  .probe = usb_probe,
  .attach = usb_attach,
  .detach = usb_detach,
  .next = NULL
};

static SceUsbdDriver composite = {
  .name = "detect_composite",
  .probe = usb_probe,
  .attach = usb_attach,
  .detach = usb_detach,
  .next = NULL
};

static SceUID g_hook;

static tai_hook_ref_t g_ksceUsbdGetDescriptor_hook;
static void *ksceUsbdGetDescriptor_patched(int device_id, int index, unsigned char bDescriptorType) {
  SceUsbdDeviceDescriptor *device = TAI_CONTINUE(void *, g_ksceUsbdGetDescriptor_hook, device_id, index, bDescriptorType);
  const uint16_t *strdesc;
  char ascii[256];
  TRACEF("ksceUsbdGetDescriptor_patched(%d, %d, 0x%02x): 0x%08X\n", device_id, index, bDescriptorType, device);
  if (bDescriptorType == 0x01 && device != NULL) {
    TRACEF("  bLength:            0x%02x\n", device->bLength);
    TRACEF("  bDescriptorType:    0x%02x\n", device->bDescriptorType);
    TRACEF("  bcdUSB:             0x%04x\n", device->bcdUSB);
    TRACEF("  bDeviceClass:       0x%02x\n", device->bDeviceClass);
    TRACEF("  bDeviceSubClass:    0x%02x\n", device->bDeviceSubClass);
    TRACEF("  bDeviceProtocol:    0x%02x\n", device->bDeviceProtocol);
    TRACEF("  bMaxPacketSize0:    0x%02x\n", device->bMaxPacketSize0);
    TRACEF("  idVendor:           0x%04x\n", device->idVendor);
    TRACEF("  idProduct:          0x%04x\n", device->idProduct);
    TRACEF("  bcdDevice:          0x%04x\n", device->bcdDevice);
    TRACEF("  iManufacturer:      0x%02x\n", device->iManufacturer);
    /*
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iManufacturer, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      TRACEF("    %s\n", ascii);
    }
    TRACEF("  iProduct:           0x%02x\n", device->iProduct);
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iProduct, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      TRACEF("    %s\n", ascii);
    }
    TRACEF("  iSerialNumber:      0x%02x\n", device->iSerialNumber);
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iSerialNumber, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      TRACEF("    %s\n", ascii);
    }
    */
    TRACEF("  bNumConfigurations: 0x%02x\n", device->bNumConfigurations);
  }
  return device;
}

static void unicodetoascii(const uint16_t *unicode, char *ascii) {
  while (*unicode > 0) {
    *ascii++ = *unicode++;
  }
}

int usb_probe(int device_id) {
  SceUsbdDeviceDescriptor *device;
  TRACEF("Device found: %x\n", device_id);

  device = ksceUsbdGetDescriptor(device_id, 0, 0x01);

  return -1;
}

int usb_attach(int device_id) {
  TRACEF("Device attached: %x\n", device_id);
  return -1;
}

int usb_detach(int device_id) {
  TRACEF("Device detached: %x\n", device_id);
  return -1;
}

int k_dump_trace(char *userbuf) {
  int state = 0;
  int count = 0;

  ENTER_SYSCALL(state);
  while (lock);
  lock = 1;
  count = at;
  ksceKernelStrncpyKernelToUser((uintptr_t)userbuf, (char *)buf, at+1);
  at = 0;
  lock = 0;
  EXIT_SYSCALL(state);
  return count;
}

int module_stop(SceSize argc, const void *pargs) {
  ksceUsbdUnregisterDriver(&driver);
  ksceUsbdUnregisterDriver(&composite);
  taiHookReleaseForKernel(g_hook, g_ksceUsbdGetDescriptor_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *pargs) {
  int ret;
  if ((ret = ksceUsbdRegisterDriver(&driver)) < 0) {
    TRACEF("ksceUsbdRegisterDriver(driver): 0x%08X\n", ret);
    return SCE_KERNEL_START_FAILED;
  }
  if ((ret = ksceUsbdRegisterCompositeLdd(&composite)) < 0) {
    ksceUsbdUnregisterDriver(&driver);
    TRACEF("ksceUsbdRegisterCompositeLdd(composite): 0x%08X\n", ret);
    return SCE_KERNEL_START_FAILED;
  }
  g_hook = taiHookFunctionExportForKernel(KERNEL_PID, &g_ksceUsbdGetDescriptor_hook, "SceUsbd", 0xA0EBCA41, 0xBC3EF82B, ksceUsbdGetDescriptor_patched);
  TRACEF("taiHookFunctionExportForKernel: 0x%08X\n", g_hook);
  return SCE_KERNEL_START_SUCCESS;
}
