/*! @file
 *
 *  @brief TSI module: control touch pad on the TWR-K70F120M.
 *
 * This module contains the functions for operating TSI.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
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
  SIM_SCGC5 |= SIM_SCGC5_TSI_MASK;      //scgs5  1<<5
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

  //Enable input pins
  TSI0_PEN |= TSI_PEN_PEN5_MASK;
  TSI0_PEN |= TSI_PEN_PEN7_MASK;
  TSI0_PEN |= TSI_PEN_PEN8_MASK;
  TSI0_PEN |= TSI_PEN_PEN9_MASK;

  //Set LP0 as clock source in active mode
  TSI0_SCANC &= ~TSI_SCANC_AMCLKS_MASK;
  TSI0_SCANC |= TSI_SCANC_AMCLKS(0);       //LP0 clock
  TSI0_SCANC &= ~TSI_SCANC_AMPSC_MASK;
  TSI0_SCANC |= TSI_SCANC_AMPSC(1);        //active mode pre-scaler = 2
  TSI0_SCANC &= ~TSI_SCANC_REFCHRG_MASK;
  TSI0_SCANC |= TSI_SCANC_REFCHRG(15);     //32uA reference OSC charge current
  TSI0_SCANC &= ~TSI_SCANC_EXTCHRG_MASK;
  TSI0_SCANC |= TSI_SCANC_EXTCHRG(8);      //18uA external OSC charge current
  TSI0_SCANC &= ~TSI_SCANC_SMOD_MASK;
  TSI0_SCANC |= TSI_SCANC_SMOD(10);        //10 cycle scan period modulus

  TSI0_GENCS |= TSI_GENCS_TSIIE_MASK;      //Enable interrupts initially
  TSI0_GENCS |= TSI_GENCS_ESOR_MASK;  //End of scan interrupt mode
  TSI0_GENCS &= ~TSI_GENCS_NSCN_MASK;
  TSI0_GENCS |= TSI_GENCS_NSCN(9);    // 10 scans per electrode
  TSI0_GENCS &= ~TSI_GENCS_PS_MASK;
  TSI0_GENCS |= TSI_GENCS_PS(2);      //electrode osc pre-scaler = 4

  //enable TSI interrupt source in NVIC
  NVICICPR2 |= (1 << 19);
  NVICISER2 |= (1 << 19);

  return bTRUE;
}

void TSI_SelfCalibration(void)
{
  int n;
  uint32union_t touchpad54,touchpad76,touchpad98;

  TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK;  //disable TSI module
  TSI0_GENCS &= ~TSI_GENCS_STM_MASK;    //software trigger scan mode
  TSI0_GENCS |= TSI_GENCS_EOSF_MASK;    //clear scan complete flag
  TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;   //enable TSI module
  TSI0_GENCS |= TSI_GENCS_SWTS_MASK;    //Write a 1 to this bit will start a scan sequence and write a 0 to this bit has no effect
  
  while(TSI_GENCS_EOSF_MASK ==(TSI0_GENCS & TSI_GENCS_EOSF_MASK));
  for(n =1;n<25000; n++)
  {
    __asm("nop");
  }

  touchpad54.l = TSI0_CNTR5;
  touchpad76.l = TSI0_CNTR7;
  touchpad98.l = TSI0_CNTR9;

  touchpad5 = touchpad54.s.Hi + calibration;
  touchpad7 = touchpad76.s.Hi + calibration;
  touchpad8 = touchpad98.s.Lo + calibration;
  touchpad9 = touchpad98.s.Hi + calibration;

  TSI0_GENCS |= TSI_GENCS_STM_MASK; //Periodical Scan
  TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;
}

BOOL readStart(void)
{
  return start;
}

BOOL readChk(void)
{
  return checkPress;
}

void writeChk(BOOL boolean)
{
  checkPress = boolean;
}

void __attribute__ ((interrupt)) TSI_ISR(void)
{
  TSI0_GENCS |= TSI_GENCS_EOSF_MASK;  //Clear interrupt flag
  if(Mode() == 1)
  {
  if((TSI0_CNTR5>>16) > touchpad5)
  {
    if(!Debounce())
      return;
    while((TSI0_CNTR5 >> 16) > touchpad5){}
    LEDs_Toggle(LED_ORANGE);
    checkPress =bTRUE;
  }
  if((TSI0_CNTR7 >> 16) > touchpad7)
  {
    if(!Debounce())
      return;
    while((TSI0_CNTR7 >> 16) > touchpad7){}
    checkPress = bTRUE;
    LEDs_Toggle(LED_GREEN);
  }
  if((uint16_t)(TSI0_CNTR9) > touchpad8)
  {
    if(!Debounce())
      return;
    while((uint16_t)(TSI0_CNTR9) > touchpad8){}
    checkPress = bTRUE;
    LEDs_Toggle(LED_YELLOW);
  }
  if((TSI0_CNTR9 >> 16) >touchpad9)
  {
    if(!Debounce())
      return;
    while((TSI0_CNTR9 >> 16) > touchpad9){}
    checkPress = bTRUE;
    LEDs_Toggle(LED_BLUE);
  }
  }
  if(Mode()==2)
  {
    if((TSI0_CNTR5 >> 16) > touchpad5)
    {
      if(!Debounce())
        return;
      while((TSI0_CNTR5 >> 16) > touchpad5){}
      start = bTRUE;
    }
    if((TSI0_CNTR7 >> 16) > touchpad7)
    {
      if(!Debounce())
        return;
      while((TSI0_CNTR7 >> 16) > touchpad7){}
      start = bTRUE;
    }
    if((uint16_t)(TSI0_CNTR9) > touchpad8)
    {
      if(!Debounce())
        return;
      while((uint16_t)(TSI0_CNTR9) > touchpad8){}
      start = bTRUE;
    }
    if((TSI0_CNTR9 >> 16) > touchpad9)
    {
      if(!Debounce())
        return;
      while((TSI0_CNTR9 >> 16) > touchpad9){}
      start = bTRUE;
    }

  }
}
