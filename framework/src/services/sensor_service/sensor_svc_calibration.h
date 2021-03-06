/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
#ifndef _SENSOR_SVC_CALIBRATION_H_
#define _SENSOR_SVC_CALIBRATION_H_

#include "os/os.h"
#include "infra/log.h"

#include "cfw/cfw.h"

/**
 * Remove the flash prop_id.
 *
 * @param[in] prop_id Properities index
 */
void sensor_clb_clean_flash(uint8_t sensor_type, uint8_t dev_id);

/**
 * Write the calibration data to flash.
 * This is a temporary api, it will be merged into ss_XX_calibration.
 *
 * @param[in] prop_id     The properties id,it mark the flash postion.
 * @param[in] data         The calibration data adderss.
 * @param[in] len          The calibration data length.
 * @return  none
 */
void sensor_clb_write_flash(uint8_t sensor_type, uint8_t dev_id, void *data,
			    uint32_t len);

/**
 * Init the calibration flash and register the flash prop_id.
 *
 * @param[in] queue        The queue
 */
void sensor_svc_clb_init(T_QUEUE queue);

#endif
