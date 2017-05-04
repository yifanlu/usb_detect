/* Copyright (C) 2017 Yifan Lu
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/usbd.h>
#include "usb_detect.h"

int printf(const char *fmt, ...);

int usb_probe(int device_id);
int usb_attach(int device_id);
int usb_detach(int device_id);

SceUsbdDriver driver = {
  .name = "detect",
  .probe = usb_probe,
  .attach = usb_attach,
  .detach = usb_detach,
  .next = NULL
};

SceUsbdDriver composite = {
  .name = "detect",
  .probe = usb_probe,
  .attach = usb_attach,
  .detach = usb_detach,
  .next = NULL
};

static void unicodetoascii(const uint16_t *unicode, char *ascii) {
  while (*unicode > 0) {
    *ascii++ = *unicode++;
  }
}

int usb_probe(int device_id) {
  SceUsbdDeviceDescriptor *device;
  const uint16_t *strdesc;
  char ascii[256];
  printf("Device found: %x\n", device_id);

  device = ksceUsbdGetDescriptor(device_id, 0, 0x01);
  if (device != NULL) {
    printf("  bLength:            0x%02x\n", device->bLength);
    printf("  bDescriptorType:    0x%02x\n", device->bDescriptorType);
    printf("  bcdUSB:             0x%04x\n", device->bcdUSB);
    printf("  bDeviceClass:       0x%02x\n", device->bDeviceClass);
    printf("  bDeviceSubClass:    0x%02x\n", device->bDeviceSubClass);
    printf("  bDeviceProtocol:    0x%02x\n", device->bDeviceProtocol);
    printf("  bMaxPacketSize0:    0x%02x\n", device->bMaxPacketSize0);
    printf("  idVendor:           0x%04x\n", device->idVendor);
    printf("  idProduct:          0x%04x\n", device->idProduct);
    printf("  bcdDevice:          0x%04x\n", device->bcdDevice);
    printf("  iManufacturer:      0x%02x\n", device->iManufacturer);
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iManufacturer, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      printf("    %s\n", ascii);
    }
    printf("  iProduct:           0x%02x\n", device->iProduct);
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iProduct, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      printf("    %s\n", ascii);
    }
    printf("  iSerialNumber:      0x%02x\n", device->iSerialNumber);
    if ((strdesc = ksceUsbdGetDescriptor(device_id, device->iSerialNumber, 0x03)) != NULL) {
      unicodetoascii(&strdesc[1], ascii);
      printf("    %s\n", ascii);
    }
    printf("  bNumConfigurations: 0x%02x\n", device->bNumConfigurations);
  }

  return 0;
}

int usb_attach(int device_id) {
  printf("Device attached: %x\n", device_id);
  return 0;
}

int usb_detach(int device_id) {
  printf("Device detached: %x\n", device_id);
  return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *pargs) {
  int ret;
  if ((ret = ksceUsbdRegisterDriver(&driver)) < 0) {
    printf("ksceUsbdRegisterDriver(driver): 0x%08X\n", ret);
    return SCE_KERNEL_START_FAILED;
  }
  if ((ret = ksceUsbdRegisterCompositeLdd(&composite)) < 0) {
    ksceUsbdUnregisterDriver(&driver);
    printf("ksceUsbdRegisterCompositeLdd(composite): 0x%08X\n", ret);
    return SCE_KERNEL_START_FAILED;
  }
  return SCE_KERNEL_START_SUCCESS;
}
