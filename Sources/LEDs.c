/*! @file
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-04-12
 */
/*!
**  @addtogroup LEDs_module LEDs module documentation
**  @{
*/
/* MODULE LEDs */

// new types
#include "types.h"
#include "IO_Map.h"
#include "LEDs.h"




BOOL LEDs_Init(void)
{
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;  // enable PORTA
  PORTA_PCR11 |= PORT_PCR_MUX(1);     // configure all led ports as GPIO
  PORTA_PCR29 |= PORT_PCR_MUX(1);
  PORTA_PCR28 |= PORT_PCR_MUX(1);
  PORTA_PCR10 |= PORT_PCR_MUX(1);
  GPIOA_PDDR |= 1 << 11;             // set PORTA as outputs
  GPIOA_PDDR |= 1 << 28;
  GPIOA_PDDR |= 1 << 29;
  GPIOA_PDDR |= 1 << 10;
  GPIOA_PSOR |= 1 << 11;             // turn all leds off
  GPIOA_PSOR |= 1 << 28;
  GPIOA_PSOR |= 1 << 29;
  GPIOA_PSOR |= 1 << 10;
  return bTRUE;
}


void LEDs_On(const TLED color)
{
  GPIOA_PCOR |= color;             // turn the led on
}


void LEDs_Off(const TLED color)
{
  GPIOA_PSOR |= color;              //turn the led off
}


void LEDs_Toggle(const TLED color)
{
  GPIOA_PTOR |= color;             // toggle the led
}
/* END LEDs */
/*!
 * @}
*/


