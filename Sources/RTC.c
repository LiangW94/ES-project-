/*! @file
 *
 *  @brief Routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the real time clock (RTC).
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-05-2
 */
/*!
**  @addtogroup RTC_module RTC module documentation
**  @{
*/
/* MODULE RTC */

// new types
#include "types.h"
#include "IO_Map.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_LDD.h"
#include "PE_Const.h"
#include "OS.h"
#include "RTC.h"


BOOL RTC_Init()
{


  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;    // enable RTC clock
  RTC_CR |= RTC_CR_OSCE_MASK;         // enable the oscillator
  RTC_CR |= RTC_CR_SC2P_MASK;         // enable 2pF capacitor
  RTC_CR |= RTC_CR_SC16P_MASK;        // enable 16pF capacitor
  RTC_LR &= ~RTC_LR_CRL_MASK;         // lock RTC
  RTC_SR &= ~ RTC_SR_TCE_MASK;        // disable timer counter for write access
  RTC_TSR = 0;                        // start timer from 0
  RTC_SR |= RTC_SR_TCE_MASK;          // enable timer counter
  RTC_IER |= RTC_IER_TSIE_MASK;       // enable timer seconds interrupt
  NVICICPR2 |= 1 << 3;                // clear any pending interrupts on RTC
  NVICISER2 |= 1 << 3;                // clear any pending interrupts on RTC
  RTCSemaphore = OS_SemaphoreCreate(0);
  return bTRUE;
}


void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  uint32_t time;
  RTC_SR &= ~ RTC_SR_TCE_MASK;       // disable timer counter for write access
  time = (hours * 3600) + (minutes * 60) + seconds;
  RTC_SR &= ~RTC_SR_TOF_MASK;        // clear timer overflow flag
  RTC_SR &= ~RTC_SR_TIF_MASK;        // clear timer invalid flag
  RTC_TSR = time;                    // set the time

  RTC_SR |= RTC_SR_TCE_MASK;         // enable timer counter

}


void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
  uint32_t time;
  time = RTC_TSR;                   // retrieve value from register
  *hours = time / 3600;             // get time
  *minutes = (time % 3600) / 60;
  *seconds = time % 60;

}

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void __attribute__ ((interrupt)) RTC_ISR(void)
{
  OS_ISREnter();                       // Start of servicing interrupt
  OS_SemaphoreSignal(RTCSemaphore);
  OS_ISRExit();                        // End of servicing interrupt


}

/* END RTC */
/*!
 * @}
*/

