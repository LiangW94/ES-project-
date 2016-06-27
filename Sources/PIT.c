/*! @file
 *
 *  @brief PIT module: Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This module contains the functions for operating the periodic interrupt timer (PIT).
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */

/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/
/* MODULE PIT */

// new types
#include "PIT.h"

uint32_t modClk;                  /*!< a 32 bits modClk*/

void (*userFunctionA)(void *);    /*!< Callback function. */

void *userArgumentsA;             /*!< Callback function. */


BOOL PIT_Init(const uint32_t moduleClk, void (*userFunction)(void *), void *userArguments)
{
  /*!save status register and disable interrupt*/
  EnterCritical();
  userFunctionA  = userFunction;          /*!make userFunction to be userFunctionA we  before*/
  userArgumentsA = userArguments;         /*!make userArguments to be userArgumentsA we  before*/
  SIM_SCGC6  |= SIM_SCGC6_PIT_MASK;       /*!enable the clock gate*/
  PIT_MCR    &= ~PIT_MCR_MDIS_MASK;       /*!enable PIT*/
  PIT_MCR    |= PIT_MCR_FRZ_MASK;         /*!freezes timer*/

  PIT_TFLG0  |= PIT_TFLG_TIF_MASK;    	  /*!set TFLG[TIF]=1 to request interrupt*/
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK ;   	  /*!Request interrupt*/
  PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;  	  /*!DISABLE THE TIMER*/
  NVICICPR2   = (1<<4);     		  /*!clear any pending interrupts on PIT:  by using table 3-5 the PIT channel0's IRQ is 68 NCIC number is 2, using function 68 mode 32*/
  NVICISER2   = (1<<4);    		  /*!enable interrupts from PIT module*/
  /*!restore status register*/
  ExitCritical();
  modClk = moduleClk;                     /*!make 32 bit module clock in the function to be modClk we give before*/
  return bTRUE;
}



void PIT_Set(const uint32_t period, const BOOL restart)
{
  if(restart)
  {
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;   		           /*!disable PIT*/
    PIT_LDVAL0  = (uint32_t)(((period*0.000000001)*modClk)-1);     /*! set a new value*/
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;   			   /*!enable the PIT*/
  }
  else
  {
    PIT_LDVAL0 = (uint32_t)(((period*0.000000001)*modClk)-1);      /*! set a new value after a trigger event*/
  }
}


void PIT_Enable(const BOOL enable)
{
  if(enable)
    PIT_TCTRL0|= PIT_TCTRL_TEN_MASK;                               /*! PIT is to be enabled.*/
  else
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;                             /*! PIT is to be disabled.*/
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT_ISR(void)
{
  (*userFunctionA) (userArgumentsA);                               /*!call the user callback function*/
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;                                  /*!set to one at the end of the timer period to this flag clear it*/
}
/* END PIT */
/*!
** @}
*/



