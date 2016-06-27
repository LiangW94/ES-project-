/*! @file
 *
 *  @brief FIFO module :Routines to implement a FIFO buffer.
 *
 *  This module contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author Liang Wang
 *  @date 2016-06-10
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/
/* MODULE FIFO */
#include "FIFO.h"


void FIFO_Init(TFIFO * const FIFO)
{
  /*! initial the value*/
  /*initial the index of the position of the oldest data in the FIFO*/
  FIFO->Start = 0;

  /* initial the index of the next available empty position in the FIFO*/
  FIFO->End = 0;

  /*initial the number of bytes currently stored in the FIFO*/
  FIFO->NbBytes = 0;
}


BOOL FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  /*!save status register and disable interrupt*/
  EnterCritical();
  /*make sure if FIFO is full,if it's full,data will not be stored*/
  if (FIFO->NbBytes == FIFO_SIZE)
    {
    /*!restore status register*/
    ExitCritical();
    return bFALSE;
    }
  /*if FIFO is not full store data*/
  else
  {
    /*store data in the next available empty position of the actual array of bytes*/
    FIFO->Buffer[FIFO->End] = data;
    /*when we stored a data FIFO->End++*/
    FIFO->End++;
    /*until it equal to 255, FIFO is full, so FIFO->End backs to 0 instead of equal to 256*/
    if (FIFO->End == FIFO_SIZE)
      FIFO->End = 0;
    /*when a data stored in FIFO,this means current stored data plus one*/
      FIFO->NbBytes ++;
    /*!restore status register*/
      ExitCritical();
      return bTRUE;
  }
}


BOOL FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
  /*!save status register and disable interrupt*/
  EnterCritical();
  /*make sure FIFO is not empty, if it's empty, return bFALSE*/
  if (FIFO->NbBytes == 0)
    {
      /*!restore status register*/
      ExitCritical();
      return bFALSE;
    }
  /*if FIFO is not empty, take out it*/
  else
  {
    /*the oldest data in the current array is put in the position where the dataPtr point*/
    *dataPtr = FIFO->Buffer[FIFO->Start];
    /*when a data is taken out of FIFO, FIFO->Start++*/
    FIFO->Start++;
    /*until it equal to 255, FIFO is empty, FIFO->End backs to 0 instead of equal to 256*/
    if (FIFO->Start == FIFO_SIZE)
      FIFO->Start = 0;
      /*when a data is get,this means current stored data minus one*/
      FIFO->NbBytes --;
      /*!restore status register*/
      ExitCritical();
      return bTRUE;
  }
}
/* END FIFO */
/*!
** @}
*/
