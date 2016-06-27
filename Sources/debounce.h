/*! @file
 *
 *  @brief Routines for setting up the debounce module.
 *
 *  @author Liang Wang
 *  @date 2016-06-19
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

// new types
#include "types.h"
#include "MK70F12.h"
/*! @brief Denounce  method before first use.
 *
 *  @return BOOL - TRUE if the Denounce was successfully initialized.
 *
 */
BOOL Debounce(void);
#endif
