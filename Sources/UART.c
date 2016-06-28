/*! @file
 *
 *  @brief UART module: I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This module contains the functions for operating the UART (serial port).
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE UART */

// new types
#include "UART.h"

BOOL UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  uint16union_t SBR;              /*!<UART baud rate*/

  register uint16_t brfa;         /*!<baud rate fine adjust*/

  SIM_SCGC5 = 1<<13;		  /*!Enable PORTE clock gate control.UART2:13*/
  PORTE_PCR16 = 3<<8;		  /*!Set MUX(bit 10 to 8) to 3(011) in PORTE16 to choose ALT3 to choose UART2_TX.*/
  PORTE_PCR17 = 3<<8;		  /*!Set MUX(bit 10 to 8) to (011)3 in PORTE17 to choose ALT3 to choose UART2_RX.*/
  SIM_SCGC4 = 1<<12;		  /*! Enable UART2 clock gate control. UART2:12.*/
  //UART2_C2 = 0x00;		  /*Turn Receiver and transmitter off.TE RE*/
  UART2_C1 = 0x00;		  /*!8 bits data, Parity function disabled. PE*/
  //UART2_C3 = 0x00;		  /*Disable interrupt.*/
  //UART2_C4 = 0x00;
  //UART2_C5 = 0x00;
  UART2_MODEM = 0x00;
  SBR.l = (moduleClk/baudRate)/16;	 /*!Baud Rate Generation.*/
  brfa = (((moduleClk<<5)/(baudRate<<4))-(SBR.l<<5));
  UART2_C4 = (UART2_C4 &~(UART_C4_BRFA(0x1F)))|UART_C4_BRFA(brfa);
  SBR.l &=(~BIT15TO13);                  /*!make bit 15 to 13 equal to 0. 0xXXXX & ~0xe000(0x1fff)it should be 000xxxxxxxxxxxxx*/
  UART2_BDH = SBR.s.Hi;                  /*!type.h BDH is the first part of uint16union_t SBR*/
  UART2_BDL = SBR.s.Lo;                  /*!type.h BDL is the second part of uint16union_t SBR*/
  UART2_C2 = UART_C2_RE_MASK | UART_C2_TE_MASK;    	/*!Transmitter Enable, no interrupt.TE RE:00001100*/

  UART2_C2 |= UART_C2_RIE_MASK;                         /*!enable receiver full interrupt*/
  UART2_C2 &= ~UART_C2_TIE_MASK;                        /*!disable transmitter interrupt*/
  NVICICPR1 = (1<<17);     /*! clear any pending interrupts on uart2:  by using table 3-5 the UART status source  IRQ is 49 NCIC number is 1, using function 49 mode 32*/
  NVICISER1 = (1<<17);     /*!enable interrupts from UART module*/

  return bTRUE;
}


BOOL UART_InChar(uint8_t * const dataPtr)
{
  /*!take out the data from RxFIFO into UART_InChar one by one*/
  return FIFO_Get(&RxFIFO,dataPtr);
}


BOOL UART_OutChar(const uint8_t data)
{
  EnterCritical();
  /*!put the data from UART_OutChar into TxFIFO one by one*/
  if(FIFO_Put(&TxFIFO, data))
  {
    UART2_C2 |= UART_C2_TIE_MASK;                     /*!enable transmitter interrupt*/
    ExitCritical();
    return bTRUE;
  }
  else
  {
    ExitCritical();
    return bFALSE;
  }
}

/*!RDRF set, data is accepted and does into RxFIFO;TDRE is set, data retrieved into TxFIFO,and sent out*/
void UART_Poll(void)
{
  uint8_t RDRF;                                        /*!< a 8 bit RDRF*/
  uint8_t TDRE;                                        /*!< a 8 bit RDRF*/
  /*make RDRF equal to 00X00000, (XXXXXXXX&00100000 = 00X00000)*/
  RDRF = (UART2_S1 & RDRFSET);
  /*if 00X00000=00100000,RDRF set, data is put into &RxFIFO*/
  if(RDRF == RDRFSET)
    FIFO_Put(&RxFIFO, UART2_D);
  /*make TDRE equal to X0000000, (XXXXXXXX&10000000 = X0000000)*/
  TDRE = (UART2_S1 & TDRESET);
  /*if X0000000=10000000,TDRE set, data is taken out of &TxFIFO*/
  if(TDRE == TDRESET)
    FIFO_Get(&TxFIFO, (uint8_t *)&UART2_D);
}
void __attribute__ ((interrupt)) UART_ISR(void)
{
  uint8_t RDRF;                                          /*!< a 8 bit RDRF*/
  uint8_t TDRE;                                          /*!< a 8 bit RDRF*/
  /*make RDRF equal to 00X00000, (XXXXXXXX&00100000 = 00X00000)*/
  RDRF = (UART2_S1 & RDRFSET);
  /*if 00X00000=00100000,RDRF set, data is put into &RxFIFO*/
  if(RDRF == RDRFSET)
    FIFO_Put(&RxFIFO, UART2_D);
  /*make TDRE equal to X0000000, (XXXXXXXX&10000000 = X0000000)*/
  TDRE = (UART2_S1 & TDRESET);
  /*if X0000000=10000000,TDRE set, data is taken out of &TxFIFO*/
  if(TDRE == TDRESET)
  {
    if(!FIFO_Get(&TxFIFO, (uint8_t *)&UART2_D))           /*if error*/
    UART2_C2 &= ~UART_C2_TIE_MASK;                        /*disable transmitter interrupt*/
  }
}

/* END UART */
/*!
** @}
*/
