/**
 * @file util.c
 * @author Greg Sleap
 * @date 5 Jul 2019
 * @brief Various utility functions
 *
 */
#include <math.h>   // for fabs
#include <stdlib.h> // for abs
#include <string.h>
/**
 *
 *  @brief Takes decimal degrees and returns the components in d (or h),m,s
 *  @param[in] degrees decimal angle in degrees.
 *  @param[out] dd pointer to an int representing degrees.
 *  @param[out] mm pointer to an int representing minutes.
 *  @param[out] ss pointer to a double representing seconds.
 *  @returns None
 */
void degrees_to_dms(double degrees, int *dd, int *mm, double *ss)
{
  *dd = (int)degrees;                 // pass back dd as the integral degrees portion
  double dmm = degrees - (double)*dd; // We should be a fractional number of degrees
  dmm = fabs(dmm) * 60.0;             // Decimal minutes
  *mm = (int)dmm;                     // pass back mm as the integral minutes portion
  double dss = dmm - (double)*mm;     // We should be a fraction number of minutes
  *ss = dss * 60.0;                   // Decimal seconds;
}

/**
 *
 *  @brief Takes decimal degrees and returns the components in d (or h),m,s
 *  @param[in] degrees decimal angle in degrees.
 *  @param[out] hh pointer to an int representing hours.
 *  @param[out] mm pointer to an int representing minutes.
 *  @param[out] ss pointer to a double representing seconds.
 *  @returns None
 */
void degrees_to_hms(double degrees, int *hh, int *mm, double *ss)
{

  *hh = (int)(degrees / 15.0);                 // pass back h as 0-23hrs (360/24)
  double dmm = degrees - ((double)*hh * 15.0); // We should be a fractional number of degrees
  dmm = dmm * 4;                               // Decimal minutes
  *mm = (int)dmm;                              // pass back mm as the integral minutes portion
  double dss = dmm - (double)*mm;              // We should be a fraction number of minutes
  *ss = dss * 60.0;                            // Decimal seconds;
}

/**
 *
 *  @brief Takes decimal degrees and returns a double in the form of ddmmss.s
 *  @param[in] hh_or_dd hours or degrees
 *  @param[in] mm minutes
 *  @param[in] ss seconds
 *  @returns double the sample angle but in ddmmss.s format
 */
double format_angle(int hh_or_dd, int mm, double ss)
{
  // e.g. 9, 53, 9.31
  // Returns 95309.31
  if (hh_or_dd < 0)
  {
    return -((abs(hh_or_dd) * 10000) + (mm * 100) + ss);
  }
  else
  {
    return (hh_or_dd * 10000) + (mm * 100) + ss;
  }
}

/**
 *
 *  @brief Returns the start byte index of the search bytes if it is found within buffer- or -1 if not found.
 *  @param[in] buffer pointer to char buffer
 *  @param[in] buffer_len length of byte buffer
 *  @param[in] search_bytes pointer to search bytes buffer
 *  @param[in] search_len length of search bytes buffer
 *  @returns -1 if search bytes not found in buffer or >0 which is the index of the start of the search bytes in the buffer.
 */
int binary_strstr(char *buffer, size_t buffer_len, char *search_bytes, size_t search_len)
{
  size_t i;
  for (i = 0; i < buffer_len - search_len; ++i)
  {
    if (memcmp(search_bytes, buffer + i, search_len) == 0)
    {
      return i; // search bytes exist in buffer- return the start index in the byte array
    }
  }
  return -1; // search bytes not in buffer
}