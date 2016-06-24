/*! @file
 *
 *  @brief Routines for setting up the Touch Sensitive Interface
 *
 *  This contains the functions for operating the Touch Sensitive Interface.
 *
 *  @author Liang Wang
 *  @date 2016-06-19
 */
/*!
**  @addtogroup TSI_module TSI module documentation
**  @{
*/
/* MODULE TSI */
#include "mk70f12.h"
#include "TSI.h"
#include "LEDs.h"
#include "debounce.h"

#define VARIANCE 250

//TSI touch plate Call-back
void readStateOfElectrodes(void * nothing);

static TDebounce TSIPlates = {
    .buttonID = BUTTON_TSI_TOUCHPLATES,
    .debounceCompleteCallbackFunction = readStateOfElectrodes,
    .debounceCompleteCallbackArguments = 0
};

TTSIMode TSI_CurrentMode;

TTSIElectrodeState TSI_LEDOrange;
TTSIElectrodeState TSI_LEDYellow;
TTSIElectrodeState TSI_LEDGreen;
TTSIElectrodeState TSI_LEDBlue;

static uint16_t TSIThresholdOrange;	/*!< Holds the threshold value that determines whether or not the 'Orange' electrode is being pressed */
static uint16_t TSIThresholdYellow;	/*!< Holds the threshold value that determines whether or not the 'Yellow' electrode is being pressed */
static uint16_t TSIThresholdGreen;	/*!< Holds the threshold value that determines whether or not the 'Green' electrode is being pressed */
static uint16_t TSIThresholdBlue;	/*!< Holds the threshold value that determines whether or not the 'Blue' electrode is being pressed */

BOOL TSI_Init(void)
{
  //Set Global external values
  TSI_CurrentMode = MODE_DEFAULT;	//Initially the tower is in default mode, so all the LEDs are available
  TSI_LEDOrange = STATE_OFF,
  TSI_LEDYellow = STATE_OFF,
  TSI_LEDGreen  = STATE_OFF,
  TSI_LEDBlue   = STATE_OFF;

  //----------------------- Initialize the TSI Module ------------------------------
  SIM_SCGC5 |= SIM_SCGC5_TSI_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

  //TSIO_CH5 - PTA4: ORANGE (ALT0)
  PORTA_PCR4 &= ~PORT_PCR_MUX_MASK;
  PORTA_PCR4 |= PORT_PCR_MUX(0);

  //TSIO_CH8 - PTB3: YELLOW (ALT0)
  PORTB_PCR3 &= ~PORT_PCR_MUX_MASK;
  PORTB_PCR3 |= PORT_PCR_MUX(0);

  //TSIO_CH7 - PTB2: GREEN  (ALT0)
  PORTB_PCR2 &= ~PORT_PCR_MUX_MASK;
  PORTB_PCR2 |= PORT_PCR_MUX(0);

  //TSIO_CH9 - PTB16: BLUE  (ALT0)
  PORTB_PCR16 &= ~PORT_PCR_MUX_MASK;
  PORTB_PCR16 |= PORT_PCR_MUX(0);

  //Disable TSI
  TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK;

  //Enable input pins
  TSI0_PEN |= TSI_PEN_PEN5_MASK;
  TSI0_PEN |= TSI_PEN_PEN7_MASK;
  TSI0_PEN |= TSI_PEN_PEN8_MASK;
  TSI0_PEN |= TSI_PEN_PEN9_MASK;

  //Set LP0 as clock source in active mode
  TSI0_SCANC &= ~TSI_SCANC_AMCLKS_MASK;
  TSI0_SCANC |= TSI_SCANC_AMCLKS(0);	//LP0 clock
  TSI0_SCANC &= ~TSI_SCANC_AMPSC_MASK;
  TSI0_SCANC |= TSI_SCANC_AMPSC(1);	//active mode pre-scaler = 2
  TSI0_SCANC &= ~TSI_SCANC_REFCHRG_MASK;
  TSI0_SCANC |= TSI_SCANC_REFCHRG(15);	//32uA reference OSC charge current
  TSI0_SCANC &= ~TSI_SCANC_EXTCHRG_MASK;
  TSI0_SCANC |= TSI_SCANC_EXTCHRG(8);	//18uA external OSC charge current
  TSI0_SCANC &= ~TSI_SCANC_SMOD_MASK;
  TSI0_SCANC |= TSI_SCANC_SMOD(10);	//10 cycle scan period modulus

  TSI0_GENCS |= TSI_GENCS_TSIIE_MASK;	//Enable interrupts initially
  TSI0_GENCS |= TSI_GENCS_ESOR_MASK; 	//End of scan interrupt mode
  //TSI0_GENCS |= TSI_GENCS_STM_MASK;	//periodic scan mode
  TSI0_GENCS &= ~TSI_GENCS_NSCN_MASK;
  TSI0_GENCS |= TSI_GENCS_NSCN(9);	// 10 scans per electrode
  TSI0_GENCS &= ~TSI_GENCS_PS_MASK;
  TSI0_GENCS |= TSI_GENCS_PS(2);	//electrode osc pre-scaler = 4

  //enable TSI interrupt source in NVIC
  NVICICPR2 |= (1 << 19);
  NVICISER2 |= (1 << 19);

  return (TSI_SelfCalibration());
}

BOOL TSI_SelfCalibration(void)
{
  BOOL calibrated = bFALSE;
  uint16_t tsiReadOrange; 	//channel 5
  uint16_t tsiReadYellow;	//channel 8
  uint16_t tsiReadGreen;   	//channel 7
  uint16_t tsiReadBlue;		//channel 9

  TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK;	//disable TSI module
  TSI0_GENCS &= ~TSI_GENCS_STM_MASK;	//software trigger scan mode
  TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	//clear scan complete flag
  TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;	//enable TSI module

  /* We wait in this loop until 2 successive reads of all
   * TSI electrodes read the same value, this will then be
   * set as the 'threshold' or 'STATE_OFF' for an electrode.
   */
  while (!calibrated)
  {
    //Trigger Scan and wait until completion
    TSI0_GENCS |= TSI_GENCS_SWTS_MASK;
    while (!(TSI0_GENCS & TSI_GENCS_EOSF_MASK));

    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	//Clear end of scan flag

    //Check to see if Read values
    if (tsiReadOrange == (TSI0_CNTR5 >> 16))
    {
      if (tsiReadYellow == (TSI0_CNTR9 & TSI_CNTR9_CTN1_MASK))
      {
	if (tsiReadGreen == (TSI0_CNTR7 >> 16))
	{
	  if(tsiReadBlue == ((TSI0_CNTR9 & TSI_CNTR9_CTN_MASK) >> 16))
	  {
	    calibrated = bTRUE;
	  }
	}
      }
    }
    else
    {
      tsiReadOrange = (TSI0_CNTR5 >> 16);
      tsiReadYellow = (TSI0_CNTR9 & TSI_CNTR9_CTN1_MASK);
      tsiReadGreen  = (TSI0_CNTR7 >> 16);
      tsiReadBlue   = ((TSI0_CNTR9 & TSI_CNTR9_CTN_MASK) >> 16);
    }
  }

  if (calibrated)
  {
    //The calibration has completed - set our threshold values
    TSIThresholdBlue   = (tsiReadBlue + VARIANCE);
    TSIThresholdGreen  = (tsiReadGreen + VARIANCE);
    TSIThresholdYellow = (tsiReadYellow + VARIANCE);
    TSIThresholdOrange = (tsiReadOrange + VARIANCE);

    TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK; //disable TSI module
    TSI0_GENCS |= TSI_GENCS_STM_MASK;	 //set periodic scan mode
    TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;	 //enable TSI module
  }

  return (calibrated);
}

void TSI_SetBaseline(void)
{
  //baseline to be adjustable on the fly
}

void TSI_SetMode(TTSIMode mode)
{
  TSI_CurrentMode = mode;
}

/*! @brief Call back routine after the TSI electrodes have finished debouncing.
 *
 *  @return void
 */
void readStateOfElectrodes(void * nothing)
{
  uint16_t tsiReadOrange = (TSI0_CNTR5 >> 16); 				//channel 5
  uint16_t tsiReadYellow = (TSI0_CNTR9 & TSI_CNTR9_CTN1_MASK);		//channel 8
  uint16_t tsiReadGreen  = (TSI0_CNTR7 >> 16);   			//channel 7
  uint16_t tsiReadBlue   = ((TSI0_CNTR9 & TSI_CNTR9_CTN_MASK) >> 16);	//channel 9

  TSI_LEDOrange = (tsiReadOrange > TSIThresholdOrange);
  TSI_LEDBlue   = (tsiReadBlue   > TSIThresholdBlue);
  TSI_LEDGreen  = (tsiReadGreen  > TSIThresholdGreen);
  TSI_LEDYellow = (tsiReadYellow > TSIThresholdYellow);
}

void __attribute__ ((interrupt)) TSI_ISR(void)
{
  TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	//Clear interrupt flag
  Debounce_Start(TSIPlates);		//Debounce
}
/* END TSI */
/*!
** @}
*/

