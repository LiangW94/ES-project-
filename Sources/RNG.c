/*! @file
 *	
 *  @brief Routines for setting up the Random Number Generator module
 *  
 *  @author Liang Wang
 *  @date 2016-06-15
 */
 
/*!
**  @addtogroup RNG_module RNG Module Documentation
**  @{
*/
/* MODULE RNG */
#include "MK70F12.h"
#include "RNG.h"

BOOL RNG_Init()
{
  SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK;	//enable RNGA Module Clock

  RNG_CR |= RNG_CR_INTM_MASK;	      //interrupts mask
  RNG_CR |= RNG_CR_GO_MASK;	        //RNG enabled
  RNG_CR |= RNG_CR_HA_MASK;	        //High Accuracy mode enabled
  RNG_CR |= RNG_CR_SLP_MASK;	      //Sleep Mode Enabled
  
  return (bTRUE);
}

uint8_t RNG_GetRandomNumber()
{
  RNG_CR &= ~RNG_CR_SLP_MASK;		            	//disable sleep mode
  while (!(RNG_SR & RNG_SR_OREG_LVL_MASK)){}	//wait for random number ready
  RNG_CR |= RNG_CR_SLP_MASK;		            	//enable sleep mode

}
/* END RNG */
/*!
** @}
*/
