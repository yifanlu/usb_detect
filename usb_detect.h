/* Copyright (C) 2017 Yifan Lu
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef USB_DETECT_H__
#define USB_DETECT_H__

#define LOG_BUF_SIZE (0x1000)

typedef struct {
} args_t;

int k_dump_trace(char *userbuf);
int dump_trace(char *buf);

#endif
