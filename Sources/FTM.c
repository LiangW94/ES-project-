/*! @file
 *
 *  @brief timer module: Routines for setting up the flexible timer module (FTM) on the TWR-K70F120M.
 *
 *  This module contains the functions for operating the flexible timer module (FTM).
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup FTM_module FTM module documentation
**  @{
*/
/* MODULE FTM */

// new types
#include "types.h"
#include "FTM.h"
#include "Cpu.h"
void(*userFunctionC)(void*);  	        // Callback function
void *userArgumentsC;        		        // Callback function
TFTMChannel*  nTimer;


BOOL FTM_Init()
{


    SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK; //enable the clock gate
    FTM0_MODE |= FTM_MODE_FTMEN_MASK; //free running counter
    FTM0_MOD = 0xFFFF;
    FTM0_SC= 0x10;			              //fix frequency clock
    NVICICPR1 = (1<<30);              //clear any pending interrupts on FTM0:  by using table 3-5 the UART status source  IRQ is 62 NCIC number is 1, using function 62 mode 32
    NVICISER1 = (1<<30);              //enable interrupts from UART module

    return bTRUE;

}


BOOL FTM_Set(const TFTMChannel* const aFTMChannel)
{

  userFunctionC = aFTMChannel->userFunction;                  //assign user callback function to local userFunction
  userArgumentsC = aFTMChannel->userArguments;                //assign user arguments to local user argument
  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_CHIE_MASK;    // enable interrupt


  switch (aFTMChannel->timerFunction)
  {
    /*!choose timer function as input compare*/
    case TIMER_FUNCTION_INPUT_CAPTURE:
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSA_MASK;    // make MSA and MSB all 0 to choose the input compare
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;
      switch(aFTMChannel->ioType.inputDetection)
      {
	case TIMER_INPUT_OFF:
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;     //for ioType.inputDetection, when off ELSB and ELSA equal to 00
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_INPUT_RISING:
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;     // when rising ELSB and ELSA equal to 01
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_INPUT_FALLING:
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;      //when falling ELSB and ELSA equal to 10
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_INPUT_ANY:
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;      // when any ELSB and ELSA equal to 11
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
	  break;
	default:
	  return bFALSE;
      }
      break;
    /*!choose timer function as output compare*/
    case TIMER_FUNCTION_OUTPUT_COMPARE:
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_MSA_MASK;    // make MSA and MSB 10 to choose the input compare
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;
      switch(aFTMChannel->ioType.outputAction)
      {
	case TIMER_OUTPUT_DISCONNECT:
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;    // for ioType.outputDetection, when off ELSB and ELSA equal to 00
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_OUTPUT_TOGGLE:
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;    // when toggle ELSB and ELSA equal to 01
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_OUTPUT_LOW:
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;     // when low ELSB and ELSA equal to 10
	  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
	  break;
	case TIMER_OUTPUT_HIGH:
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;     // when high ELSB and ELSA equal to 11
	  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
	  break;
	default:
	  return bFALSE;
      }
      break;
    default:
      return bFALSE;
  }
  return bTRUE;
}


BOOL FTM_StartTimer(const TFTMChannel* const aFTMChannel, const int RATE)
{
  if(aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
  {
    FTM0_CnV(aFTMChannel->channelNb) =FTM0_CNT+RATE;        // add 1 second delay
    return bTRUE;
  }
  return bFALSE;
}



void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  (*userFunctionC) (userArgumentsC);                          //call back function
    FTM0_CnSC(0) &=~ FTM_CnSC_CHF_MASK;                       //clear the interrupt flag for channel 0
}
/* END FTM */
/*!
** @}
*/
