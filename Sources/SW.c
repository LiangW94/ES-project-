/*! @file
 *
 *  @brief switch module: control two switches on the TWR-K70F120M.
 *
 * This module contains the functions for operating two switches.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */


// new types
#include "SW.h"
static int mode = 0;

BOOL SW_Init(void)
{
  //initialize switch 1,2clock gating
  SIM_SCGC5 |= (SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK); 
  
  //Initialize Switch 1
  PORTD_PCR0  &= ~PORT_PCR_MUX_MASK; //Set type to GPIO - The pins are set as input by default
  PORTD_PCR0  |= PORT_PCR_MUX(1);
  PORTD_PCR0  |= PORT_PCR_ISF_MASK;  //Setup interrupts for each Pin
  PORTD_PCR0  |= PORT_PCR_IRQC(10);
  PORTD_PCR0 |= PORT_PCR_PE_MASK;    //Enable Pull-up Resistors
  PORTD_PCR0 |= PORT_PCR_PS_MASK;    //Enable the corresponding pin
  NVICICPR2 |= (1 << 26);            //Initialize NVIC
  NVICISER2 |= (1 << 26);

  //Initialize Switch 2
  PORTE_PCR26 &= ~PORT_PCR_MUX_MASK;
  PORTE_PCR26 |= PORT_PCR_MUX(1);
  PORTE_PCR26 |= PORT_PCR_ISF_MASK;   //Clear pending interrupt flags (write 1 to clear)
  PORTE_PCR26 |= PORT_PCR_IRQC(10);   //Interrupt on pin falling-edge
  PORTE_PCR26 |= PORT_PCR_PE_MASK;    //Enable Pull-up Resistors
  PORTE_PCR26 |= PORT_PCR_PS_MASK;    //Enable the corresponding pin
  NVICICPR2 |= (1 << 27);             //PORTE
  NVICISER2 |= (1 << 27);
  return (bTRUE);
}

int Mode(void)
{
  return mode;
}

void WriteMode(int newmode)
{
  mode = newmode;
}

void __attribute__ ((interrupt)) SW0_ISR(void)
{
  int i;
  TLED color[4] = {LED_BLUE, LED_GREEN, LED_YELLOW, LED_ORANGE};
  PORTD_ISFR = 0xFFFFFFFF;
  if(Debounce())
    return;
  mode = 1;
  for(i=0;i<4;i++)
    LEDs_Off(color[i]);
  while((GPIOD_PDIR & 1)==0){}
}

void __attribute__ ((interrupt)) SW1_ISR(void)
{
  static const int MASK = 4000000;
  PORTE_ISFR = 0xFFFFFFFF;
  if(Debounce())
    return;
  mode = 2;
  while(((GPIOE_PDIR>>26)& 1)==0){}
}
