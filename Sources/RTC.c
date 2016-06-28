/*! @file
 *
 *  @brief RTC module: Routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  This module contains the functions for operating the real time clock (RTC).
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup RTC_module RTC module documentation
**  @{
*/
/* MODULE RTC */

// new types
#include "types.h"
#include "MK70F12.h"
#include "PE_Types.h"
#include "LEDs.h"
#include "RTC.h"
void (*userFunctionB)(void *);                  /*!< Callback function. */
void *userArgumentsB;                           /*!< Callback function. */


BOOL RTC_Init(void (*userFunction)(void *), void *userArguments)
{
  /*!save status register and disable interrupt*/
  EnterCritical();
  userFunctionB = userFunction;                /*!assign user callback function to local userFunction.*/
  userArgumentsB = userArguments;              /*!assign user arguments to local user argument.*/
  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;             /*!enable the clock gate*/

  RTC_CR |= RTC_CR_OSCE_MASK;                  /*!enable 32.768kHz oscillator*/
  RTC_CR |= RTC_CR_SC2P_MASK;                  /*!enable the additional load*/
  RTC_CR |= RTC_CR_SC16P_MASK;

  RTC_LR &= ~RTC_LR_CRL_MASK;                  /*!lock the control register.*/
  RTC_SR |= RTC_SR_TCE_MASK;                   /*!enable the counter.*/
  RTC_IER |= RTC_IER_TSIE_MASK;                /*!Seconds interrupt enable.*/
  RTC_SR &= ~RTC_SR_TCE_MASK;                  /*!disable counter to make sure that the RTC_TSR and PRC_TPR can be read and written.*/
  RTC_TPR = 0;                                 /*!initial the time*/
  RTC_TSR = 0;
  RTC_SR |= RTC_SR_TCE_MASK;                   /*!enable the counter*/
  NVICICPR2 = (1<<3);     		       /*! clear any pending interrupts on :  by using table 3-5 the Seconds interrupt's IRQ is 67 NCIC number is 2, using function 67 mode 32*/
  NVICISER2 = (1<<3);    		       /*!enable interrupts from RTC module*/
  /*!restore status register*/
  ExitCritical();
  return bTRUE;

}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
   uint32_t setSecond;                          /*!< a 32 bit time in second*/

   setSecond = 3600*hours +60*minutes+ seconds; /*!Separate set hours, minutes and seconds*/
   RTC_SR &= ~RTC_SR_TCE_MASK;                  /*!disable counter to make sure that the RTC_TSR and PRC_TPR can be read and written*/

   RTC_TPR = 0;                                 /*!set TPR 0 to set TSR*/
   RTC_TSR = setSecond;                         /*!write time into TSR*/
   RTC_SR &= ~RTC_SR_TOF_MASK;                  /*!time overflow has not occurred*/
   RTC_SR &= ~RTC_SR_TIF_MASK;                  /*!time is valid*/
   RTC_SR |= RTC_SR_TCE_MASK;                   /*!enable the counter*/
}


void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
  uint32_t getSecond;                           /*!< a 32 bit time in second*/

  RTC_SR &= ~RTC_SR_TCE_MASK;                   /*!disable the counter*/
  getSecond =  RTC_TSR;                         /*!read from TSR*/
  *hours = (getSecond/3600);                    /*!calculate the hour*/
  if(*hours >=24)                               /*!if hour >= 24, make hour 0*/
    *hours=0;
  *minutes = (getSecond%3600)/60;               /*!calculate the minute*/
  *seconds = getSecond%60;                      /*!calculate the seconds*/
  RTC_SR |= RTC_SR_TCE_MASK;                    /*!enable the counter*/
}


void __attribute__ ((interrupt)) RTC_ISR(void)
{
  (*userFunctionB) (userArgumentsB);         /*!call the user callback function*/
}
/* END RTC */
/*!
** @}
*/



