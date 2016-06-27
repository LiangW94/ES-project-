/*! @file
 *
 *  @brief Routines for setting up the debounce module.
 *
 *  @author Liang Wang
 *  @date 2016-06-19
 */
/*!
**  @addtogroup debounce_module debounce module documentation
**  @{
*/
/* MODULE debounce */

#include "debounce.h"


BOOL Debounce(void)
{
  static uint16_t counter =0;
  if (counter == 10)
  {
    counter = 0;
    return bFALSE;
  }
  else
    counter ++;
  return bTRUE;
}



/* END debounce */
/*!
** @}
*/
