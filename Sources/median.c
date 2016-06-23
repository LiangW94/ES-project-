/*! @file
 *
 *  @brief Median filter.
 *
 *  This contains the functions for performing a median filter on byte-sized data.
 *
 *  @author PMcL
 *  @date 2015-10-12
 */
/*!
**  @addtogroup median_module median module documentation
**  @{
*/
/* MODULE median */

#include "median.h"

uint8_t Median_Filter3(const uint8_t n1, const uint8_t n2, const uint8_t n3)
{

  if (n1 > n2)
  {
    if (n2 > n3)
      return (n2);
    else
    {
      if (n1 > n3)
        return (n3);
      else
        return (n1);
    }
  }
  else
  {
    if (n3 > n2)
      return (n2);
    else
    {
      if (n1 > n3)
        return (n1);
      else
        return (n3);
    }
  }
}
/* END median */
/*!
 * @}
*/



