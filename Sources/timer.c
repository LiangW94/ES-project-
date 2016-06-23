/*! @file
 *
 *  @brief Routines for setting up the flexible timer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the flexible timer module (FTM).
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-05-4
 */
/*!
**  @addtogroup timer_module timer module documentation
**  @{
*/
/* MODULE timer */

// new types
#include "types.h"
#include "IO_Map.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_LDD.h"
#include "PE_Const.h"
#include "timer.h"
#include "OS.h"


uint8_t channel;
TTimerFunction function;
BOOL Timer_Init()
{

  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
  FTM0_CNTIN = 0;                       // Initialize FTM0 routine
  FTM0_MOD = 0xffff;
  FTM0_CNT = 0;
  FTM0_SC = 0x10;                       // fixed frequency clock
  NVICICPR1 |= 1 << 30;                 // clear any pending interrupts on FTM
  NVICISER1 |= 1 << 30;                 // enable interrupts on FTM
  FTMSemaphore = OS_SemaphoreCreate(0);
}


BOOL Timer_Set(const TTimer* const aTimer)
{

  channel = aTimer -> channelNb;

  FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_CHIE_MASK;                // enable channel interrupt on
  if (aTimer -> timerFunction == TIMER_FUNCTION_INPUT_CAPTURE)
    {
      FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_MSB_MASK;    // make MSA and MSB all 0 to choose the input compare
      FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_MSA_MASK;
      if (aTimer -> ioType.inputDetection == TIMER_INPUT_OFF)
	{
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSB_MASK;   //for ioType.inputDetection, when off ELSB and ELSA equal to 00
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSA_MASK;
	}
      else if (aTimer -> ioType.inputDetection == TIMER_INPUT_RISING)
	{
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSB_MASK;
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSA_MASK;
	}
      else if (aTimer -> ioType.inputDetection == TIMER_INPUT_FALLING)
	{
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSB_MASK;          
	 FTM0_CnSC (aTimer -> channelNb) & ~FTM_CnSC_ELSA_MASK;   
	}
      else if (aTimer -> ioType.inputDetection == TIMER_INPUT_ANY)
	{
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSB_MASK;
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSA_MASK;
	}
    }
    if (aTimer -> timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)  //choose timer function as output compare
    {
      FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_MSB_MASK;
      FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_MSA_MASK;       //make MSA and MSB 10 to choose the input compare
      if (aTimer -> ioType.outputAction == TIMER_OUTPUT_DISCONNECT)
	{
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSB_MASK;        //for ioType.outputDetection, when off ELSB and ELSA equal to 00
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSA_MASK;
	}
      else if (aTimer -> ioType.outputAction == TIMER_OUTPUT_TOGGLE)
	{
	 FTM0_CnSC (aTimer -> channelNb) &= ~FTM_CnSC_ELSB_MASK;     //when toggle ELSB and ELSA equal to 01
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSA_MASK;
	}
      else if (aTimer -> ioType.outputAction == TIMER_OUTPUT_LOW)
	{
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSB_MASK;        //when low ELSB and ELSA equal to 10
	 FTM0_CnSC (aTimer -> channelNb) & ~FTM_CnSC_ELSA_MASK;
	}
      else if (aTimer -> ioType.outputAction == TIMER_OUTPUT_HIGH)
	{
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSB_MASK;           //when high ELSB and ELSA equal to 11
	 FTM0_CnSC (aTimer -> channelNb) |= FTM_CnSC_ELSA_MASK;
	}
    }
    return bTRUE;
}



BOOL Timer_Start(const TTimer* const aTimer)
{
  if (aTimer -> timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)  //starts timer only for output compare mode
    {
       FTM0_CnV (aTimer -> channelNb ) = aTimer -> initialCount + FTM0_CNT + 24414;          // set channel to generate 1 sec interrupt
       return bTRUE;
    }
  return bFALSE;

}



void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  FTM0_CnSC (channel) &=~ FTM_CnSC_CHF_MASK;              // clear channel interrupt flag
  OS_ISREnter();                                          // Start of servicing interrupt
  OS_SemaphoreSignal(FTMSemaphore);
  OS_ISRExit();                                           // End of servicing interrupt

}
/* END timer */
/*!
 * @}
*/




