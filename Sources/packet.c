/*! @file
 *
 *  @brief packet module: Routines to implement packet encoding and decoding for the serial port.
 *
 *  This module contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup packet_module packet module documentation
**  @{
*/
/* MODULE packet */
#include "packet.h"

TPacket Packet;     /*!< Packet as a TPacket*/

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return BOOL - TRUE if the packet module was successfully initialized.
 */

BOOL Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
   return UART_Init(baudRate, moduleClk);       /*use UART to get the packet*/
}
/*! @brief Attempts to get a packet from the received data.
 *
 *  @return BOOL - TRUE if a valid packet was received.
 */
BOOL Packet_Get(void)
{
  static int i = 0;           

  /*!a loop to get a packet*/
  switch(i)
  {
    case 0:
      if (UART_InChar(&Packet_Command))
      {
        i++;
      }
      break;
    case 1:
      if (UART_InChar(&Packet_Parameter1))
      {
        i++;
      }
      break;
    case 2:
      if (UART_InChar(&Packet_Parameter2))
      {
        i++;
      }
      break;
    case 3:
      if (UART_InChar(&Packet_Parameter3))
      {
        i++;
      }
      break;
    case 4:
      if (UART_InChar(&Packet_Checksum))
      {
        i++;
      }
      break;
    /*!check the packet, if not, keep looping*/
    case 5:
      if (Packet_Checksum != (Packet_Command^Packet_Parameter1^ Packet_Parameter2^Packet_Parameter3))
      {
        Packet_Command = Packet_Parameter1;
        Packet_Parameter1 = Packet_Parameter2;
        Packet_Parameter2 = Packet_Parameter3;
        Packet_Parameter3 = Packet_Checksum;
        i = 4;
      }
      /*!if a packet is got successfully, start another loop*/
      else
      {
        i=0;
        return bTRUE;
      }
      break;
    default:
      break;
  }
  return bFALSE;
}


BOOL Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  /*!put the packet into TFIFO*/
  return (UART_OutChar(command) &&
	  UART_OutChar(parameter1) &&
	  UART_OutChar(parameter2) &&
	  UART_OutChar(parameter3) &&
	  UART_OutChar(command^parameter1^parameter2^parameter3));

}
/* END packet */
/*!
** @}
*/
