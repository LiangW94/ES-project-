/*
 /*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *
 *  @author Liang Wang Thanawat Parthomsakulrat
 *  @date 2016-03-25
 */
/*!
**  @addtogroup packet_module packet module documentation
**  @{
*/
/* MODULE packet */

#include "types.h"
#include "UART.h"
#include "packet.h"
#include "Cpu.h"
#include "PE_Types.h"


TPacket Packet;


BOOL Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  return UART_Init(baudRate, moduleClk);
}


BOOL Packet_Get(void)
{

  static uint8_t PacketChecksum;

  static int stage = 0;                    // variable to get a byte at a time
  if (stage == 0)
  {
    if (UART_InChar(&Packet_Command))       // it will get first byte at first run
      stage++;
      return bFALSE;
  }
  if (stage == 1)
  {
    if (UART_InChar(&Packet_Parameter1))    // it will get second byte at second run
      stage++;
      return bFALSE;
  }
  if (stage == 2)
  {
    if (UART_InChar(&Packet_Parameter2))    // it will get third byte at third run
      stage++;
      return bFALSE;
  }
  if (stage == 3)
  {
    if (UART_InChar(&Packet_Parameter3))    // it will get fourth byte at fourth run
      stage++;
      return bFALSE;
  }
  if (stage == 4)
  {
    if (UART_InChar(&PacketChecksum))      // it will get fifth byte at fifth run
      stage++;
      return bFALSE;
  }
  if (stage == 5)                          // for timing issues this stage is to build a packet
  {
    uint8_t check = (Packet_Command^Packet_Parameter1)^(Packet_Parameter2^Packet_Parameter3);
    if (PacketChecksum == check)
    {
      stage = 0;
      return bTRUE;
    }
    else
    {
       Packet_Command  = Packet_Parameter1;
       Packet_Parameter1 = Packet_Parameter2;
       Packet_Parameter2 = Packet_Parameter3;
       Packet_Parameter3 = PacketChecksum;
       stage = 4;
    }
  }
  return bFALSE;
}


BOOL Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{

  return (UART_OutChar(command) &&
         UART_OutChar(parameter1) &&
         UART_OutChar(parameter2) &&
         UART_OutChar(parameter3) &&
         UART_OutChar(command^parameter1^parameter2^parameter3));

}
/* END packet */
/*!
 * @}
*/
