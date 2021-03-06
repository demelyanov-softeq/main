/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include <os/os.h>
#include <rtc.h>
#include "infra/tcmd/handler.h"
#include "util/assert.h"

#define UINT32_ANSWER_LENGTH 11 // Corresponds to INT_MAX (10 digits) + \0
#define DOUBLE_UINT32_ANSWER_LENGTH 22 // Corresponds to twice INT_MAX (20 digits) + space + \0
#define RTC_FINISHED_STR_LEN 13 // Corresponds to string 'Rtc finished' + space
#define RTC_DRV_NAME "RTC"

#define RTC_ARG_NO   3
#define RTC_TIME_IDX 2

static volatile uint32_t test_rtc = 0;
static volatile uint32_t tcmd_user_initial_rtc_val = 0;
static volatile uint32_t tcmd_user_alarm_rtc_val = 0;
static volatile uint32_t tcmd_alarm_rtc_read = 0;
static volatile bool alarm_pending = false;

/*
 * @addtogroup infra_tcmd
 * @{
 */

/*
 * @defgroup infra_tcmd_rtc RTC Test Commands
 * Interfaces to support RTC Test Commands.
 * @{
 */

#ifdef CONFIG_INTEL_QRK_RTC_TCMD
/*
 * Function called at elapsed RTC alarm time.
 *
 * @param[in]   val         input parameter for callback function
 */
static void test_rtc_interrupt_fn(struct device *rtc_dev)
{
	struct tcmd_handler_ctx *ctx =
		(struct tcmd_handler_ctx *)rtc_dev->driver_data;
	char answer[DOUBLE_UINT32_ANSWER_LENGTH];

	tcmd_alarm_rtc_read = rtc_read(rtc_dev);
	test_rtc++;
	snprintf(answer, DOUBLE_UINT32_ANSWER_LENGTH, "%u %u",
		 tcmd_user_alarm_rtc_val,
		 tcmd_alarm_rtc_read);
	alarm_pending = false;
	TCMD_RSP_FINAL(ctx, answer);
}

/*
 * Test command to set initial RTC time: rtc set <time>
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The context to pass back to responses
 */
void rtc_set(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct rtc_config config = { 0 };
	uint32_t keys = 0;
	struct device *rtc_dev;

	if (argc == RTC_ARG_NO && isdigit(argv[RTC_TIME_IDX][0])) {
		rtc_dev = device_get_binding(RTC_DRV_NAME);
		assert(rtc_dev != NULL);
		keys = irq_lock();
		tcmd_user_initial_rtc_val = (uint32_t)strtoul(
			argv[RTC_TIME_IDX], NULL, 10);
		config.init_val = tcmd_user_initial_rtc_val;
		irq_unlock(keys);
		rtc_set_config(rtc_dev, &config);

		TCMD_RSP_FINAL(ctx, NULL);
	} else {
		TCMD_RSP_ERROR(ctx, "Usage: rtc set <initial_time>");
	}
}
DECLARE_TEST_COMMAND_ENG(rtc, set, rtc_set);

/*
 * Test command to read current RTC: rtc read
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The context to pass back to responses
 */
void rtc_read_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	char answer[UINT32_ANSWER_LENGTH];
	struct device *rtc_dev;

	rtc_dev = device_get_binding(RTC_DRV_NAME);
	assert(rtc_dev != NULL);
	snprintf(answer, UINT32_ANSWER_LENGTH, "%u", rtc_read(rtc_dev));
	TCMD_RSP_FINAL(ctx, answer);
}
DECLARE_TEST_COMMAND_ENG(rtc, read, rtc_read_tcmd);

/*
 * Test command to set RTC Alarm time, and start RTC: rtc alarm <rtc_alarm_time>
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The context to pass back to responses
 */
void rtc_alarm_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	struct rtc_config config = { 0 };
	struct device *rtc_dev;
	uint32_t keys = 0;

	if (argc == RTC_ARG_NO && isdigit(argv[RTC_TIME_IDX][0])) {
		keys = irq_lock();
		tcmd_user_alarm_rtc_val =
			(uint32_t)strtoul(argv[RTC_TIME_IDX], NULL,
					  10);
		test_rtc = 0;
		config.alarm_val = tcmd_user_alarm_rtc_val;
		config.alarm_enable = true;
		config.cb_fn = test_rtc_interrupt_fn;
		alarm_pending = true;
		irq_unlock(keys);

		rtc_dev = device_get_binding(RTC_DRV_NAME);
		assert(rtc_dev != NULL);
		rtc_dev->driver_data = (void *)ctx;
		rtc_enable(rtc_dev);
		rtc_set_config(rtc_dev, &config);
		rtc_set_alarm(rtc_dev, config.alarm_val);
	} else {
		TCMD_RSP_ERROR(ctx, "Usage: rtc alarm <alarm_time>");
	}
}
DECLARE_TEST_COMMAND_ENG(rtc, alarm, rtc_alarm_tcmd);

/*
 * Test command to get status of RTC(not config, running, finished): rtc status
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The context to pass back to responses
 */
void rtc_status(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	char answer[RTC_FINISHED_STR_LEN + DOUBLE_UINT32_ANSWER_LENGTH];
	uint32_t keys = 0;
	bool local_alarm_pending = false;
	uint32_t local_test_rtc = 0;
	uint32_t local_tcmd_user_alarm_rtc_val = 0;
	uint32_t local_tcmd_alarm_rtc_read = 0;

	keys = irq_lock();
	local_alarm_pending = alarm_pending;
	local_test_rtc = test_rtc;
	local_tcmd_user_alarm_rtc_val = tcmd_user_alarm_rtc_val;
	local_tcmd_alarm_rtc_read = tcmd_alarm_rtc_read;
	irq_unlock(keys);

	if (local_alarm_pending) {
		TCMD_RSP_FINAL(ctx, "Rtc is running.");
	} else if (local_test_rtc) {
		snprintf(answer, RTC_FINISHED_STR_LEN +
			 DOUBLE_UINT32_ANSWER_LENGTH,
			 "Rtc finished %u %u", local_tcmd_user_alarm_rtc_val,
			 local_tcmd_alarm_rtc_read);
		TCMD_RSP_FINAL(ctx, answer);
	} else {
		TCMD_RSP_ERROR(ctx, "Rtc not configured.");
	}
}
DECLARE_TEST_COMMAND_ENG(rtc, status, rtc_status);
#endif

/*
 * @}
 *
 * @}
 */
