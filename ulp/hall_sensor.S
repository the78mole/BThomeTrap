/* ULP Example: Read hall sensor in deep sleep

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */


/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files 
 */

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"

	/* Configure the number of ADC samples to average on each measurement.
     For convenience, make it a power of 2. */
    .set adc_oversampling_factor_log, 2
	.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)

	.set threshold, 70
	.set hallthreshold, 0
	.set halloff, 0
    .set hallval, 0
	.set hallphase0p, 0
	.set hallphase0n, 0
	.set hallphase0d, 0
	.set hallphase1p, 0
	.set hallphase1n, 0
	.set hallphase1d, 0
	.set pwr3val, 0
	.set pwr5val, 0
	.set silentcnt, 1
	.set detected, 0

	/* Define variables, which go into .bss section (zero-initialized data) */
	.bss
	.global Offset
Offset: .long 0	

    .global hallval
hallval: .long 0

	.global hallthreshold
hallthreshold: .long 0

	.global halloff
halloff: .long 0

	.global pwr3val
pwr3val: .long 0

	.global hallphase0p
hallphase0p: .long 0

	.global hallphase0n
hallphase0n: .long 0

	.global hallphase0d
hallphase0d: .long 0

	.global hallphase1p
hallphase1p: .long 0

	.global hallphase1n
hallphase1n: .long 0

	.global hallphase1d
hallphase1d: .long 0

	.global pwr5val
pwr5val: .long 0

	.global silentcnt
silentcnt: .long 0

	.global detected
detected: .long 0

	.global detcnt
detcnt: .long 0

	/* Code goes into .text section */
	.text
	.global entry
entry:

	/* SENS_XPD_HALL_FORCE = 1, hall sensor force enable, XPD HALL is controlled by SW */
	WRITE_RTC_REG(SENS_SAR_TOUCH_CTRL1_REG, SENS_XPD_HALL_FORCE_S, 1, 1)

	/* RTC_IO_XPD_HALL = 1, xpd hall, Power on hall sensor and connect to VP and VN */
	WRITE_RTC_REG(RTC_IO_HALL_SENS_REG, RTC_IO_XPD_HALL_S, 1, 1)

	/* SENS_HALL_PHASE_FORCE = 1, phase force, HALL PHASE is controlled by SW */
	WRITE_RTC_REG(SENS_SAR_TOUCH_CTRL1_REG, SENS_HALL_PHASE_FORCE_S, 1, 1)

	/* RTC_IO_HALL_PHASE = 0, phase of hall sensor */
	WRITE_RTC_REG(RTC_IO_HALL_SENS_REG, RTC_IO_HALL_PHASE_S, 1, 0)

	/* SENS_FORCE_XPD_SAR, Force power up */
	WRITE_RTC_REG(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR_S, 2, SENS_FORCE_XPD_SAR_PU)

	/* r0 will be used for result calculation */
	/* r1, r2 will be used as accumulator */
	move r1, 0
	move r2, 0	
	/* r3 will be used for variable addresses and temp values */
	stage_rst
measure0:
	/* Get vp0 */
	adc r3, 0, 1
	/* Add vp0 to R1 */
	add r1, r1, r3

	/* Get vn0 */
	adc r3, 0, 4
	/* Add vn0 to R2 */
	add r2, r2, r3

	/* Increment counter */
	//add r0, r0, 1
	stage_inc 1
	/* Jump to phase 1 measure only if counter > 4 */
	jumps measure0, adc_oversampling_factor, lt

	/* Getting averages on R1 and R2 */
	rsh r1, r1, adc_oversampling_factor_log
	rsh r2, r2, adc_oversampling_factor_log

	// Store values in variables
	move r3, hallphase0p
    st r1, r3, 0
	move r3, hallphase0n
    st r2, r3, 0
	sub r0, r2, r1
	move r3, hallphase0d
	st r0, r3, 0

	/* Resetting loop counter */
	// move r0, 0
	stage_rst

	/* RTC_IO_HALL_PHASE = 1, phase of hall sensor */
	/* This reverts the current in the hall sensor to compensate for it's offset (a little bit) */
	WRITE_RTC_REG(RTC_IO_HALL_SENS_REG, RTC_IO_HALL_PHASE_S, 1, 1)

measure1:
	/* Get vp0 */
	adc r3, 0, 1
	/* Add vp0 to R1 */
	add r1, r1, r3

	/* Get vn0 */
	adc r3, 0, 4
	/* Add vn0 to R2 */
	add r2, r2, r3

	/* Increment counter */
	stage_inc 1
	/* Jump to phase 1 measure only if counter > 4 */
	jumps measure1, adc_oversampling_factor, lt

	/* Getting averages on R1 and R2 */
	rsh r1, r1, adc_oversampling_factor_log
	rsh r2, r2, adc_oversampling_factor_log

	move r3, hallphase1p
    st r1, r3, 0
	move r3, hallphase1n
    st r2, r3, 0
	sub r0, r2, r1
	move r3, hallphase1d
	st r0, r3, 0

	/* Get the stored value from first phase */
	move r3, hallphase0d
	ld r1, r3, 0
	/* Subtract it */
	sub r0, r1, r0

	/* Get the offset and subtract it */
	move r3, halloff
	ld r2, r3, 0
	sub r0, r0, r2 /* 99 */

	/* If result crossed 0, decrease offset by one, otherwise increase */
	jump saveoff, eq
	jump offs_dec, ov
	add r2, r2, 2

offs_dec:
	sub r2, r2, 1

saveoff:
	st r2, r3, 0

makeabs:
	jumpr export_outcome, 32767, le
	move r3, 65535
	sub r0, r3, r0
	add r0, r0, 1


export_outcome:
	/* Store the measured hall value */
	move r3, hallval
	st r0, r3, 0

threshold_compare:
	move r3, hallthreshold
	ld r2, r3, 0
	sub r2, r2, r0
	jump setdetection, ov
 	//jump setdetection, hallthreshold, ge

check_wake:
	move r3, silentcnt
	ld r0, r3, 0
	sub r0, r0, 1
	jump wakecpu, eq
	st r0, r3, 0
	halt

setdetection:
	move r3, detected
	move r0, 1
	st r0, r3, 0
	move r3, detcnt
	/* increase detection counter */
	ld r0, r3, 0
	add r0, r0, 1
	st r0, r3, 0

wakecpu:
	adc r1, 0, 6  // ADC1_CH5, GPIO33
	wait 10
	move r3, pwr5val
	st r1, r3, 0
	adc r1, 0, 7  // ADC1_CH6, GPIO34
	wait 10
	move r3, pwr3val
	st r1, r3, 0
	wake
	halt