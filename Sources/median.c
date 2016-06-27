/*! @file
 *
 *  @brief Median module.
 *
 * This module contains the structure and "methods" for getting median.
 *
 *  @author Liang Wang
 *  @date 2016-06-28
 */
/*!
**  @addtogroup Median_module Median module documentation
**  @{
*/
/* MODULE median */
#include "median.h"
/*! @brief Median filters 3 bytes.
 *
 *  @param n1 is the first  of 3 bytes for which the median is sought.
 *  @param n2 is the second of 3 bytes for which the median is sought.
 *  @param n3 is the third  of 3 bytes for which the median is sought.
 */
uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{
  int a,b;
  if(n1 >= n2)
  {
    a = n1;
    b = n2;
  }

  else
  {
    a = n2;
    b = n1;
  }
  if(n3 >= a)
    return a;
  else if(n3 > b)
    return n3;
  return b;
}
/* END median */
/*!
** @}
*/
