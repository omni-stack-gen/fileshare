/*
 * debug.h - debug operations
 *
 * Copyright (C) 2019, LomboTech Co.Ltd.
 * Author: lomboswer <lomboswer@lombotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <libgen.h>

#include <utils/log.h>

#define ALOGV(...) log_output(LOG_LEVEL_VERBOSE, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGD(...) log_output(LOG_LEVEL_DEBUG,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGI(...) log_output(LOG_LEVEL_INFO,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGW(...) log_output(LOG_LEVEL_WARN,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGE(...) log_output(LOG_LEVEL_ERROR,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#endif
