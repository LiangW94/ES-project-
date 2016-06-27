/*! @file
 *
 *  @brief TSI module: control touch pad on the TWR-K70F120M.
 *
 * This module contains the functions for operating TSI.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
#ifndef TSI_H
#define TSI_H

// new types
#include "types.h"
#include "MK70F12.h"
#include "LEDs.h"
#include "debounce.h"
#include "RNG.h"
#include "SW.h"
#include "FTM.h"
/*! @brief Sets up the TSI before first use.
 *
 *  @return BOOL - TRUE if the TSI was successfully initialized.
 *
 */
BOOL TSI_Init(void);

/*! @brief Sets up TSI SelfCalibration.
 *
 */
void TSI_SelfCalibration(void);
//void TSI_Start(void);


/*! @brief read check press .
 *
 */
BOOL readChk(void);

/*! @brief read start.
 *
 */
BOOL readStart(void);

/*! @brief write check press .
 *
 */
void writeChk(BOOL boolean);

/*! @brief Interrupt service routine for the TSI.
 *
 *  If  touch the touch pad, toggle LED.
 *  @note Assumes the TSI has been initialized.
 */
void __attribute__ ((interrupt)) TSI_ISR(void);
#endif
