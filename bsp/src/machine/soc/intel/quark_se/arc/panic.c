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

#include <string.h>
#include <stdbool.h>
#include "machine.h"
#include "os/os.h"
#include "project_mapping.h"
#include "panic_quark_se.h"
#include "infra/version.h"
#include "infra/time.h"
#ifdef CONFIG_IPC
#include "infra/ipc.h"
#endif

/**
 * \brief Maximum stack dump size
 */
#define PANIC_ARC_STACK_SIZE 160 /*!< ARC stack dump size (in bytes) */

#define PANIC_NOTIFICATION_ERROR_CODE 1 /*!< Error code for panic triggered by notification */

extern char __ram_phys_end[];
const struct arcv2_panic_data *arcv2_panic_data =
	(struct arcv2_panic_data *)__ram_phys_end - 1;
static volatile bool is_panic = false;

static volatile int is_panic_notification = 0;

#define IN_RANGE(val, range_min, range_max) \
	((val) >= (range_min) && (val) < (range_max))

void panic_handler(void)
{
	irq_lock();

	struct arcv2_panic_arch_data *arc_info =
		(struct arcv2_panic_arch_data *)&arcv2_panic_data->arch_data;
	struct panic_data_footer *footer =
		(struct panic_data_footer *)&arcv2_panic_data->footer;

	footer->struct_size = sizeof(struct panic_data_footer) +
			      sizeof(struct arcv2_panic_arch_data);
	// Copy stack to panic RAM location if sp is valid
	if (IN_RANGE(arc_info->sp, ARC_RAM_START_ADDR,
		     ARC_RAM_START_ADDR + ARC_RAM_SIZE * 1024) ||
	    IN_RANGE(arc_info->sp, ARC_DCCM_START_ADDR,
		     ARC_DCCM_START_ADDR + CONFIG_QUARK_SE_ARC_DCCM_SIZE)) {
		uint8_t *stack_dump = (uint8_t *)arc_info -
				      PANIC_ARC_STACK_SIZE;
		// Align sp pointer on 32bits to avoid alignement issues
		memcpy(stack_dump, (uint32_t *)((arc_info->sp + 3) & (~3)),
		       PANIC_ARC_STACK_SIZE);

		footer->struct_size += PANIC_ARC_STACK_SIZE;
	}

	footer->arch = ARC_CORE;
	footer->flags = PANIC_DATA_FLAG_FRAME_VALID |
			PANIC_DATA_FLAG_FRAME_BLE_AVAILABLE;
	footer->magic = PANIC_DATA_MAGIC;
	footer->reserved = 0;
	footer->time = get_uptime_ms();
	memcpy(&footer->build_cksum, (const void *)version_header.hash,
	       sizeof(version_header.hash));
	footer->struct_version = version_header.version;

	if (!is_panic) {
		is_panic = true;
		if (!is_panic_notification) {
			// Notify master core that a panic occured
			panic_notify(0);
		} else {
			// Notify master core that panic notification has been processed
			panic_done();
		}
	}
	// Stop core
	while (1) ;
}

void handle_panic_notification(int core_id)
{
	// Prevent panic handler to send a notification if receiver has already crashed
	is_panic_notification = 1;
	panic(PANIC_NOTIFICATION_ERROR_CODE);
}

void panic_notify(int timeout)
{
#ifdef CONFIG_IPC
	// Send IPC panic notification
	if (!is_panic_notification) {
		// On ARC core, ipc_request_sync_int is the equivalent to mailbox register write
		// and thus will generate an QRK interrupt as if it were a GPIO
		ipc_request_notify_panic(ARC_CORE);
	}
#endif
}

void panic_done()
{
}
