 /*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-03-25
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/
/* MODULE FIFO */

// new types
#include "types.h"
#include "stdlib.h"
#include "FIFO.h"
#include "OS.h"
#include "Cpu.h"
#include "PE_Types.h"



void FIFO_Init(TFIFO * const FIFO)
{

  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->NbBytes = 0;
  FIFO->SpaceAvailable  = OS_SemaphoreCreate(FIFO_SIZE); // FIFO not full semaphore initialized to FIFO_SIZE to use it as a counting semaphore
  FIFO->ItemsAvailable  = OS_SemaphoreCreate(0);        // FIFO not empty semaphore initialized to 0 to signal occurrence of events
  FIFO->BufferAccess = OS_SemaphoreCreate(1);           // BufferAccess semaphore initialized to 1 to access shared resource


}


BOOL FIFO_Put(TFIFO * const FIFO,  const uint8_t data)
{
  OS_ERROR error;
  (void)OS_SemaphoreWait(FIFO->BufferAccess, 0);
  error = OS_SemaphoreWait(FIFO->SpaceAvailable, 1);   // Wait for space to become available
  if (error == OS_TIMEOUT)
  {
    (void)OS_SemaphoreSignal(FIFO->BufferAccess);
    return bFALSE;
  }
  FIFO->Buffer[FIFO->End] = data;                     // put data into FIFO
  FIFO->End = (FIFO->End+1) % FIFO_SIZE;              // if FIFO is full, restart from 0, otherwise increase
  FIFO->NbBytes++;                                    // increase size
  (void)OS_SemaphoreSignal(FIFO->ItemsAvailable );    // Increment the number of items available
  (void)OS_SemaphoreSignal(FIFO->BufferAccess);       // Relinquish exclusive access to the FIFO
  return bTRUE;
}


BOOL FIFO_Get(TFIFO * const FIFO, uint8_t volatile * const dataPtr)
{
  OS_ERROR error;
  (void)OS_SemaphoreWait(FIFO->BufferAccess, 0);
  error = OS_SemaphoreWait(FIFO->ItemsAvailable, 1); // Wait for items to become available
  if (error == OS_TIMEOUT)
  {
    (void)OS_SemaphoreSignal(FIFO->BufferAccess);
    return bFALSE;
  }
  *dataPtr = FIFO->Buffer[FIFO->Start];               // retrieve data from FIFO
  FIFO->Start = (FIFO->Start+1) % FIFO_SIZE;          // if FIFO reaches end, restart from beginning, otherwise increase
  FIFO->NbBytes--;                                    // decrease size
  (void)OS_SemaphoreSignal(FIFO->SpaceAvailable);     // Increment the number of spaces available
  (void)OS_SemaphoreSignal(FIFO->BufferAccess);       // Relinquish exclusive access to the FIFO
  return bTRUE;

}




/* END FIFO */
/*!
 * @}
*/


