/*! @file
 *
 *  @brief RGN module: generate the random number .
 *
 * This module contains the functions for operating RNG.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */

#ifndef RNG_H
#define RNG_H

// new types
#include "types.h"
#include "MK70F12.h"
/*! @brief Sets up the RNG before first use.
 *
 *  @return BOOL - TRUE if the RNG was successfully Set up.
 */
BOOL RNG_Init(void);
/*! @brief get RNG number.
 *
 *  @return RNG_OR if it is not empty.
 *  @note Assumes that RNG was successfully Set up.
 */
uint32_t RNG_Number(void);
#endif
