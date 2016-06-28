/*! @file
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author Liang Wang
 *  @date 2016-04-12
 */
/*!
**  @addtogroup LEDs_module LEDs module documentation
**  @{
*/
/* MODULE LEDs */

// new types
#include "types.h"
#include "LEDs.h"


BOOL LEDs_Init(void)
{
  /*!enable the clock gate*/
  SIM_SCGC5 |=SIM_SCGC5_PORTA_MASK;
  PORTA_PCR11 |=PORT_PCR_MUX(1);
  PORTA_PCR28 |=PORT_PCR_MUX(1);
  PORTA_PCR29 |=PORT_PCR_MUX(1);
  PORTA_PCR10 |=PORT_PCR_MUX(1);
  GPIOA_PDDR |=1<<11;
  GPIOA_PDDR |=1<<28;
  GPIOA_PDDR |=1<<29;
  GPIOA_PDDR |=1<<10;

  return bTRUE;
}


void LEDs_On(const TLED color)
{
  switch(color)
  {
    /*!turn on orange led*/
    case LED_ORANGE:
      //GPIOA_PDDR |=1<<11;
      GPIOA_PCOR |=1<<11;
      break;
      /*!turn on yellow led*/
    case LED_YELLOW:
     // GPIOA_PDDR |=1<<28;
      GPIOA_PCOR |=1<<28;
      break;
      /*!turn on green led*/
    case LED_GREEN:

     // GPIOA_PDDR |=1<<29;
      GPIOA_PCOR |=1<<29;
      break;
      /*!turn on blue led*/
    case LED_BLUE:

     // GPIOA_PDDR |=1<<10;
      GPIOA_PCOR |=1<<10;
      break;
    default:
      break;
  }
}

void LEDs_Off(const TLED color)
{
  switch(color)
  {
  /*!turn off orange led*/
    case LED_ORANGE:

      //GPIOA_PDDR |=1<<11;
      GPIOA_PSOR |=1<<11;
      break;
      /*!turn off yellow led*/
    case LED_YELLOW:

     // GPIOA_PDDR |=1<<28;
      GPIOA_PSOR |=1<<28;
      break;
      /*!turn off green led*/
    case LED_GREEN:

      //GPIOA_PDDR |=1<<29;
      GPIOA_PSOR |=1<<29;
      break;
  /*!turn off blue led*/
    case LED_BLUE:

     // GPIOA_PDDR |=1<<10;
      GPIOA_PSOR |=1<<10;
      break;
    default:
      break;
  }
}



void LEDs_Toggle(const TLED color)
{
  switch(color)
  {
  /*!toggle orange led*/
    case LED_ORANGE:

      //GPIOA_PDDR |=1<<11;
      GPIOA_PTOR |=1<<11;
      break;
  /*!toggle yelow led*/
    case LED_YELLOW:

      //GPIOA_PDDR |=1<<28;
      GPIOA_PTOR |=1<<28;
      break;
   /*!toggle green led*/
    case LED_GREEN:

      //GPIOA_PDDR |=1<<29;
      GPIOA_PTOR |=1<<29;
      break;
   /*!toggle blue led*/
    case LED_BLUE:

      //GPIOA_PDDR |=1<<10;
      GPIOA_PTOR |=1<<10;
      break;
    default:
      break;
  }
}
/* END LEDs */
/*!
** @}
*/



