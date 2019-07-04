/**
 * @file metafitsreader.c
 * @author Greg Sleap
 * @date 14 Jun 2019
 * @brief This is the code that handles reading fits files
 *
 */
#include <assert.h>
#include <errno.h>
#include <fitsio.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "metafitsreader.h"
#include "multilog.h"

/**
 *
 *  @brief Opens a fits file for reading.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in,out] fptr Pointer to a pointer of the openned fits file.
 *  @param[in] filename Full path/name of the file to be openned.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int open_fits(dada_client_t *client, fitsfile **fptr, const char *filename)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  int status = 0;

  if (fits_open_file(fptr, filename, READONLY, &status))
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "open_fits(): Error openning fits file %s. Error: %d -- %s\n", filename, status, error_text);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/**
 *
 *  @brief Reads fields from a metafits (FITS) file.
 *  @param[in] fitsfile Pointer to a an openned metafits file.
 *  @param[in] metafits_info structure to write into. 
  *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int read_metafits(dada_client_t *client, fitsfile *fptr_metafits, metafits_s *mptr)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  int status = 0;
  
  // GPSTIME / OBSID
  long obsid = 0;
  char key_gpstime[FLEN_KEYWORD] = "GPSTIME";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_gpstime);
  if ( fits_read_key(fptr_metafits, TLONG, key_gpstime, &obsid, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_gpstime, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->obsid = obsid;

  // MJD
  double mjd = 0;
  char key_mjd[FLEN_KEYWORD] = "MJD";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_mjd);
  if ( fits_read_key(fptr_metafits, TDOUBLE, key_mjd, &mjd, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_mjd, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->mjd = mjd;

  // RA
  double ra = 0;
  char key_ra[FLEN_KEYWORD] = "RA";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_ra);
  if ( fits_read_key(fptr_metafits, TDOUBLE, key_ra, &ra, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_ra, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->ra = ra;

  // DEC
  double dec = 0;
  char key_dec[FLEN_KEYWORD] = "DEC";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_dec);
  if ( fits_read_key(fptr_metafits, TDOUBLE, key_dec, &dec, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_dec, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->dec = dec;

  // Altitude
  double altitude = 0;
  char key_altitude[FLEN_KEYWORD] = "ALTITUDE";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_altitude);
  if ( fits_read_key(fptr_metafits, TDOUBLE, key_altitude, &altitude, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_altitude, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->altitude = altitude;

  // Azimuth
  double azimuth = 0;
  char key_azimuth[FLEN_KEYWORD] = "AZIMUTH";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_azimuth);
  if ( fits_read_key(fptr_metafits, TDOUBLE, key_azimuth, &azimuth, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_azimuth, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->azimuth = azimuth;

  // FILENAME (source/object/obs name)
  char filename[FLEN_VALUE];
  char key_filename[FLEN_KEYWORD] = "FILENAME";

  multilog(log, LOG_INFO, "Reading %s from metafits\n", key_filename);
  if ( fits_read_key(fptr_metafits, TSTRING, key_filename, filename, NULL, &status) )
  {
    char error_text[30]="";
    fits_get_errstatus(status, error_text);
    multilog(log, LOG_ERR, "Error reading metafits key: %s in file %s. Error: %d -- %s\n", key_filename, fptr_metafits->Fptr->filename, status, error_text);
    return EXIT_FAILURE;
  }
  mptr->filename = strdup(filename);  
  
  multilog(log, LOG_INFO, "metafits->OBSID: %ld\n", mptr->obsid);
  multilog(log, LOG_INFO, "metafits->RA: %f\n", mptr->ra);
  multilog(log, LOG_INFO, "metafits->DEC: %f\n", mptr->dec);
  multilog(log, LOG_INFO, "metafits->ALT: %f\n", mptr->altitude);
  multilog(log, LOG_INFO, "metafits->AZ: %f\n", mptr->azimuth);
  multilog(log, LOG_INFO, "metafits->FILENAME: %s\n", mptr->filename);
  
  return (EXIT_SUCCESS);
}

/**
 *
 *  @brief Closes the fits file.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in,out] fptr Pointer to a pointer to the fitsfile structure.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int close_fits(dada_client_t *client, fitsfile **fptr)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  multilog(log, LOG_DEBUG, "close_fits(): Starting.\n");

  int status = 0;

  if (*fptr != NULL)
  {
    if (fits_close_file(*fptr, &status))
    {
      char error_text[30]="";
      fits_get_errstatus(status, error_text);
      multilog(log, LOG_ERR, "close_fits(): Error closing fits file. Error: %d -- %s\n", status, error_text);
      return EXIT_FAILURE;
    }
    else
    {
      *fptr = NULL;
    }
  }
  else
  {
    multilog(log, LOG_WARNING, "close_fits(): Fits file is already closed.\n");
  }

  return(EXIT_SUCCESS);
}