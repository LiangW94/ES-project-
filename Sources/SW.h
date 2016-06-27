/*! @file
 *
 *  @brief switch module: control two switches on the TWR-K70F120M.
 *
 * This module contains the functions for operating two switches.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
#ifndef SW_H
#define SW_H

// new types
#include "types.h"
#include "MK70F12.h"
#include "debounce.h"
#include "LEDs.h"

/*! @brief Sets up the switch before first use.
 *
 *  Enables portD and portE.
 *  @return BOOL - TRUE if the SW was successfully initialized.
 */
BOOL SW_Init(void);


/*! @brief read mode value.
 *
 *  @return mode value.
 */
int Mode(void);


/*! @brief write mode value.
 *
 */
void WriteMode(int newmode);

/*! @brief Interrupt service routine for the SW0.
 *
 *  If switch0 is pressed, change into touch toggle game.
 *  @note Assumes the SW has been initialized.
 */
void __attribute__ ((interrupt)) SW0_ISR(void);


/*! @brief Interrupt service routine for the SW1.
 *
 *  If switch1 is pressed, change into memory game mode.
 *  @note Assumes the SW has been initialized.
 */
void __attribute__ ((interrupt)) SW1_ISR(void);
#endif
