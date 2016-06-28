/*! @file
 *
 *  @brief switch module: control two switches on the TWR-K70F120M.
 *
 * This module contains the functions for operating two switches.
 *
 *  @author Liang Wang
 *  @date 2016-06-15
 */
/*!
**  @addtogroup SW_module SW module documentation
**  @{
*/
/* MODULE SW */

// new types
#include "SW.h"
static int mode = 0;

BOOL SW_Init(void)
{
  //Initialize Switch 1
  SIM_SCGC5|= SIM_SCGC5_PORTD_MASK; //initialize switch clock gating
  PORTD_PCR0 |= PORT_PCR_MUX(1);
  PORTD_PCR0 |= PORT_PCR_PE_MASK;   //Enable Pull-up Resistors
  PORTD_PCR0 |= PORT_PCR_PS_MASK;   //Enable the corresponding pin
  PORTD_PCR0 |= PORT_PCR_IRQC(8);
  NVICICPR2 = (1<<26);              //Initialize NVIC
  NVICISER2 = (1<<26);
  //Initialize Switch 2
  SIM_SCGC5|= SIM_SCGC5_PORTE_MASK; //initialize switch clock gating
  PORTE_PCR26 |= PORT_PCR_MUX(1);
  PORTE_PCR26 |= PORT_PCR_PE_MASK;  //Enable Pull-up Resistors
  PORTE_PCR26 |= PORT_PCR_PS_MASK;  //Enable the corresponding pin
  PORTE_PCR26 |= PORT_PCR_IRQC(8);
  NVICICPR2 = (1 << 27);            //Initialize NVIC
  NVICISER2 = (1 << 27);
  return bTRUE;
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
  if (Debounce())
    return;
  mode = 1;                    //mode 1 touchtoggle
  for (i = 0;i < 4;i++)             //turn off leds
    LEDs_Off(color[i]);
  while ((GPIOD_PDIR & 1) == 0){} //PORT D, Pin 0
}

void __attribute__ ((interrupt)) SW1_ISR(void)
{
  static const int MASK = 4000000;
  PORTE_ISFR = 0xFFFFFFFF;
  if (Debounce())
    return;
  mode = 2;                         //mode 2 game
  while (((GPIOE_PDIR >> 26) & 1) == 0){} //PORT E, Pin 26
}

/* END SW */
/*!
** @}
*/
