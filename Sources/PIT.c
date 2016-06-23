/*! @file
 *
 *  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the periodic interrupt timer (PIT).
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-05-02
 */
/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/
/* MODULE PIT */
// new types
#include "types.h"
#include "IO_Map.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_LDD.h"
#include "PE_Const.h"
#include "OS.h"
#include "PIT.h"

uint32_t moduleClk1;


BOOL PIT_Init(const uint32_t moduleClk)
{


  moduleClk1 = moduleClk;
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;    // enable PIT clock
  PIT_MCR &= ~PIT_MCR_MDIS_MASK;      // clock for standard PIT timer is enabled
  PIT_MCR |= PIT_MCR_FRZ_MASK;        // timer freezes in debug mode
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;     // the flag is off
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;   // enable PIT interrupts

  NVICICPR2 |= 1 << 4;                // clear any pending interrupts on PIT
  NVICISER2 |= 1 << 4;                // enable interrupts on PIT
  PITSemaphore = OS_SemaphoreCreate(0);

  return bTRUE;
}


void PIT_Set(const uint32_t period, const BOOL restart)
{
  if (restart)
  {
    PIT_Enable(bFALSE);          // disable the timer
    PIT_LDVAL0 = ((period/1000)*(moduleClk1/1000000)) - 1; //period is in nano second
    PIT_Enable(bTRUE);           // enable timer
  }
  else
  {
    PIT_LDVAL0 = ((period/1000)*(moduleClk1/1000000)) - 1;

  }

}


void PIT_Enable(const BOOL enable)
{
  if (enable)
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;      // enable timer
  else
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;     // disable timer
}


void __attribute__ ((interrupt)) PIT_ISR(void)
{
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;            // turn off the flag
  OS_ISREnter();                             // Start of servicing interrupt
  OS_SemaphoreSignal(PITSemaphore);
  OS_ISRExit();                              // End of servicing interrupt
}
/* END PIT */
/*!
 * @}
*/



