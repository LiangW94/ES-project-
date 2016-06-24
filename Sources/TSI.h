/*! @file
 *
 *  @brief Routines for setting up the Touch Sensitive Interface 
 *
 *  This contains the functions for operating the Touch Sensitive Interface.
 *
 *  @author Liang Wang
 *  @date 2016-06-19
 */

#ifndef TSI_H
#define TSI_H

// new types
#include "types.h"

typedef enum
{
  MODE_DEFAULT,
  MODE_TOUCH_TOGGLE,
  MODE_MEMORY_GAME
} TTSIMode;

typedef enum
{
  STATE_OFF,
  STATE_PRESSED
} TTSIElectrodeState;

extern TTSIMode TSI_CurrentMode;		/*!< Used to store the current TSI mode. */
extern TTSIElectrodeState TSI_LEDOrange;	/*!< Used to indicate the current state of the Orange LED electrode. */
extern TTSIElectrodeState TSI_LEDYellow;	/*!< Used to indicate the current state of the Yellow LED electrode. */
extern TTSIElectrodeState TSI_LEDGreen;		/*!< Used to indicate the current state of the Green LED electrode. */
extern TTSIElectrodeState TSI_LEDBlue;		/*!< Used to indicate the current state of the Blue LED electrode. */

/*! @brief Sets up the TSI before first use.
 *
 *  @param gameSeed - An array of 32 randomly generated numbers to 'seed' the memory game.
 *  @return BOOL - TRUE if the TSI was successfully initialized.
 */
BOOL TSI_Init(void);

/*! @brief Calibrate the TSI.
 *
 *  The TSI performs a self-calibration.
 *  @return void
 */
BOOL TSI_SelfCalibration(void);

/*! @brief Set the baseline capacitance value of the TSI.
 *
 *  The TSI baseline capacitance is set.
 *  @return void
 */
void TSI_SetBaseline(void);

/*! @brief Set the Mode of the TSI.
 *
 *  @param mode - the new mode of the TSI
 *  @return void
 */
void TSI_SetMode(TTSIMode mode);

/*! @brief TSI interrupt.
 *
 *  @return void
 */
void __attribute__ ((interrupt)) TSI_ISR(void);
#endif
