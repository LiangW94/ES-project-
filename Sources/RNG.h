/*! @file
 *	
 *  @brief Routines for setting up the Random Number Generator module
 *  
 *  @author Liang Wang
 *  @date 2016-06-15
 */

#ifndef RNG_H
#define RNG_H

#include "types.h"


/*! @brief sets up the RNG module before first use
 *  
 *  @return BOOL - TRUE if the module was succesfully initialised
 */
BOOL RNG_Init();

/*! @brief Gets a random number from the RNG
 *
 *  @return RNG_OR - the random number
 */
uint8_t RNG_GetRandomNumber();
#endif
