/*! @file
 *
 *  @brief TSI module: control touch pad on the TWR-K70F120M.
 *
 * This module contains the functions for operating TSI.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup TSI_module TSI module documentation
**  @{
*/
/* MODULE TSI */

#include "TSI.h"
static uint16_t touchpad5;
static uint16_t touchpad7;
static uint16_t touchpad8;
static uint16_t touchpad9;
static uint16_t calibration = 0x50;
static BOOL start = bFALSE;
static BOOL checkPress = bTRUE;

BOOL TSI_Init(void)
{
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
  SIM_SCGC5 |= SIM_SCGC5_TSI_MASK;			//scgs5  1<<5

  TSI0_GENCS |= TSI_GENCS_NSCN(0x09); 		// 10 times per electrode.
  TSI0_GENCS |= TSI_GENCS_TSIIE_MASK;           //Enable interrupts initially
  TSI0_GENCS |= TSI_GENCS_PS(0x02);		//Electrode Oscillator Frequency divided by 4
  TSI0_GENCS |= TSI_GENCS_ESOR_MASK;            //End-of-Scan interrupt is allowed.
  TSI0_SCANC |= TSI_SCANC_EXTCHRG(0x08);        //18 uA charge current:1000
  TSI0_SCANC |= TSI_SCANC_REFCHRG(0x0F);        //32 uA charge current
  TSI0_SCANC |= TSI_SCANC_SMOD(0x0A);           //choose the SMOD to Scan Period Modulus  value is 10.
  TSI0_SCANC |= TSI_SCANC_AMPSC(0x01);          //Input Clock Source divided by 2
  TSI0_PEN |= TSI_PEN_PEN5_MASK;                //Enable input pins
  TSI0_PEN |= TSI_PEN_PEN7_MASK;
  TSI0_PEN |= TSI_PEN_PEN8_MASK;
  TSI0_PEN |= TSI_PEN_PEN9_MASK;

  NVICICPR2 = (1 << 19);                           //enable TSI interrupt source in NVIC
  NVICISER2 = (1 << 19);
  TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;
  return bTRUE;
}
void TSI_SelfCalibration(void)
{
  int n;
  uint32union_t touchpad54,touchpad76,touchpad98;
  //Write a 1 to this bit will start a scan sequence and write a 0 to this bit has no effect
  TSI0_GENCS |= TSI_GENCS_SWTS_MASK;
  while (TSI_GENCS_EOSF_MASK == (TSI0_GENCS & TSI_GENCS_EOSF_MASK));
  for (n = 1;n < 25000; n++)
  {
    __asm("nop");
  }

  touchpad54.l = TSI0_CNTR5;
  touchpad76.l = TSI0_CNTR7;
  touchpad98.l = TSI0_CNTR9;
  //The calibration has completed then set threshold values
  touchpad5 = touchpad54.s.Hi + calibration;
  touchpad7 = touchpad76.s.Hi + calibration;
  touchpad8 = touchpad98.s.Lo + calibration;
  touchpad9 = touchpad98.s.Hi + calibration;

  TSI0_GENCS |= TSI_GENCS_STM_MASK;           //set periodic scan mode
  TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;         //enable TSI module
}

BOOL TSI_ReadStart(void)
{
  return start;
}
BOOL TSI_ReadChk(void)
{
  return checkPress;
}

void TSI_WriteChk(BOOL boolean)
{
  checkPress = boolean;
}

void __attribute__ ((interrupt)) TSI_ISR(void)
{
  TSI0_GENCS |= TSI_GENCS_EOSF_MASK;            //Clear interrupt flag
  if (Mode() == 1)                              //mode 1 is touch toggle
  {

   if ((TSI0_CNTR5>>16) > touchpad5)            //channel 5
   {
    if (!Debounce())
      return;

    while ((TSI0_CNTR5 >> 16) > touchpad5){}
    LEDs_Toggle(LED_ORANGE);                   //toggle the led
    checkPress = bTRUE;                        //mark the flag as bTrue
   }

  if ((TSI0_CNTR7>>16) > touchpad7)            //channel 7
  {
    if (!Debounce())
      return;
    while ((TSI0_CNTR7 >> 16) > touchpad7){}
    LEDs_Toggle(LED_GREEN);                   //toggle the led
    checkPress = bTRUE;                       //mark the flag as bTrue
  }

  if ((uint16_t)(TSI0_CNTR9) > touchpad8)     //channel 9
  {
    if (!Debounce())
      return;
    while ((uint16_t)(TSI0_CNTR9) > touchpad8){}
    LEDs_Toggle(LED_YELLOW);                 //toggle the led
    checkPress = bTRUE;                      //mark the flag as bTrue
  }

  if ((TSI0_CNTR9 >> 16) > touchpad9)
  {
    if (!Debounce())
      return;
    while ((TSI0_CNTR9 >> 16) > touchpad9){}
    LEDs_Toggle(LED_BLUE);                  //toggle the led
    checkPress = bTRUE;                     //mark the flag as bTrue
  }
 }

  if (Mode() == 2)                          //mode 2 is game
  {
    if ((TSI0_CNTR5 >> 16) > touchpad5)
    {
      if (!Debounce())
        return;
      while((TSI0_CNTR5 >> 16) > touchpad5){}
      start = bTRUE;
    }
    if ((TSI0_CNTR7 >> 16) > touchpad7)
    {
      if (!Debounce())
        return;
      while ((TSI0_CNTR7 >> 16) > touchpad7){}
      start = bTRUE;
    }
    if ((uint16_t)(TSI0_CNTR9) > touchpad8)
    {
      if (!Debounce())
        return;
      while((uint16_t)(TSI0_CNTR9) > touchpad8){}
      start = bTRUE;
    }
    if ((TSI0_CNTR9 >> 16) > touchpad9)
    {
      if (!Debounce())
        return;
      while ((TSI0_CNTR9 >> 16) > touchpad9){}
      start = bTRUE;
    }

  }
}

/* END TSI */
/*!
** @}
*/
