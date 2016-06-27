/*! @file
 *
 *  @brief RGN module: generate the random number .
 *
 * This module contains the functions for operating RNG.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */

#include "RNG.h"

BOOL RNG_Init(void)
{
  SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK;
  RNG_CR &= ~RNG_CR_SLP_MASK; 		     /* set SLP bit to 0 - "RNGA is in normal mode" */
  RNG_CR |= RNG_CR_INTM_MASK;               //Interrupt Mask,Masks the triggering of an error interrupt
  RNG_CR |= RNG_CR_HA_MASK;		    /* set HA bit to 1 - "Enables notification of security violations" */
  RNG_CR |= RNG_CR_GO_MASK;    		    /* set GO bit to 1 - "output register loaded with data" */
  RNG_ER =0;
  return bTRUE;
}

uint32_t RNG_Number(void)
{
  while(RNG_SR & RNG_SR_OREG_LVL_MASK == 0) /*IF EMMPTY*/
  {}
  return RNG_OR;
}
