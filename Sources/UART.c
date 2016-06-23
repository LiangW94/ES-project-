/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-03-28
 */
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE UART */


#include "types.h"
#include "FIFO.h"
#include "IO_Map.h"
#include "UART.h"

#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_LDD.h"
#include "PE_Const.h"
#include "Cpu.h"
#include "OS.h"

#define THREAD_STACK_SIZE 500   // Arbitrary thread stack size - big enough for stacking of interrupts and OS use.

static TFIFO TxFIFO;            // transmit FIFO
static TFIFO RxFIFO;            // receive FIFO

static OS_ECB *ReceiveSemaphore;     // Binary semaphore for signaling receiving of data
static OS_ECB *TransmitSemaphore;    // Binary semaphore for signaling that data transmission

static uint32_t ReceiveThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); // The stack for the receive thread
static uint32_t TransmitThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));// The stack for the transmit thread.

/*! @brief Receives a byte from UART and puts it into the FIFO
 *
 *  @param pData is not used but is required by the OS to create a thread.
 */
static void ReceiveThread(void *pData)
{
  for (;;)
  {
    (void)OS_SemaphoreWait(ReceiveSemaphore, 0);
    FIFO_Put(&RxFIFO, UART2_D);

  }

}
/*! @brief Transmits a byte into UART
 *
 *  @param pData is not used but is required by the OS to create a thread.
 */
static void TransmitThread(void *pData)
{
  for(;;)
  {
    (void)OS_SemaphoreWait(TransmitSemaphore, 0);
    if(!FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D))
      UART2_C2 &= ~UART_C2_TIE_MASK;                  // turn transmit interrupt off

  }

}
BOOL UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  OS_ERROR error;                              // Thread content
  int16union_t sbr;                            // a variable to hold SBR value
  uint16_t brfa;
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;           // enable UART2
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;           // enable PORTE
  PORTE_PCR16 |= PORT_PCR_MUX(3);              // set as UART2_TX
  PORTE_PCR17 |= PORT_PCR_MUX(3);              // set as UART2_RX
  UART2_C1 = 0x00;
  UART2_C2 = 0x0c;                             // set the transmitter and receiver on
  UART2_C2 |= UART_C2_RIE_MASK;
  UART2_C3 = 0x00;
  UART2_C4 = 0x00;
  UART2_C5 = 0x00;
  UART2_MODEM = 0x00;
  sbr.l = ((moduleClk/baudRate) / 16);         // calculate SBR from moduleclk and baudrate
  brfa = ((moduleClk << 5) / (baudRate << 4)) - (sbr.l << 5);          // calculate baud rate fine adjust
  UART2_C4 |= UART_C4_BRFA (brfa);             // set the BRFA in UART2_C4
  sbr.l &= 0x1fff;                             // get [12:0] of SBR
  UART2_BDH = sbr.s.Hi;                        // set 5 bits to BDH
  UART2_BDL = sbr.s.Lo;                        // set 8 bits to BDL
  UART2_C2 |= UART_C2_RIE_MASK;                // turn receive interrupt on
  UART2_C2 &= ~UART_C2_TIE_MASK;               // turn transmit interrupt off
  NVICICPR1 |= (1 << 17);                      // Clear any pending interrupts on UART
  NVICISER1 |= (1 << 17);                      // Enable interrupts from UART module
  FIFO_Init(&RxFIFO);                          // initialize the two FIFOs
  FIFO_Init(&TxFIFO);
  ReceiveSemaphore = OS_SemaphoreCreate(0);   // Receive semaphore initialized to 0
  TransmitSemaphore = OS_SemaphoreCreate(0);  // Transmit semaphore initialized to 0

  error = OS_ThreadCreate(ReceiveThread,     // 2nd highest priority thread
			  NULL,
			  &ReceiveThreadStack[THREAD_STACK_SIZE - 1],
			  3);

  error = OS_ThreadCreate(TransmitThread,    // 3rd highest priority thread
			  NULL,
			  &TransmitThreadStack[THREAD_STACK_SIZE - 1],
			  4);
  return bTRUE;
}


BOOL UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);          // Gets oldest byte from RxFIFO
}


BOOL UART_OutChar(const uint8_t data)
{
  if (FIFO_Put(&TxFIFO, data))
  {
    UART2_C2 |= UART_C2_TIE_MASK;              // enable transmit interrupt
    return bTRUE;
  }
  return bFALSE;


}



void UART_Poll(void)
{
  if (UART2_S1 & UART_S1_RDRF_MASK)                   // check if there is incoming data
    FIFO_Put(&RxFIFO, UART2_D);
  if (UART2_S1 & UART_S1_TDRE_MASK)                   // check if there is outgoing data
    FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D);
}


void __attribute__ ((interrupt)) UART_ISR(void)
{
  OS_ISREnter();                                        // Start of servicing interrupt

  if (UART2_C2 & UART_C2_RIE_MASK)
  {
    if (UART2_S1 & UART_S1_RDRF_MASK)                   // check if there is incoming data
      (void)OS_SemaphoreSignal(ReceiveSemaphore);       // Signal receive thread

  }
  if (UART2_C2 & UART_C2_TIE_MASK)
  {
    if (UART2_S1 & UART_S1_TDRE_MASK)                   // check if there is outgoing data
      (void)OS_SemaphoreSignal(TransmitSemaphore);      // Signal transmit thread

  }

  OS_ISRExit();                                         // End of servicing interrupt
}
/* END UART */
/*!
 * @}
*/



