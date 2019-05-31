/**
 * @file filwriter.c
 * @author Greg Sleap
 * @date 15 May 2018
 * @brief This is the code that handles writing fil files
 *
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "filfile.h"
#include "filwriter.h"
#include "multilog.h"

/**
 *
 *  @brief Creates a blank new fil file called 'filename' and populates it with data from the psrdada header.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[out] fptr pointer to the pointer of the fil file created.
 *  @param[in] filename Full path and name of the fil file to create.
 *  @returns EXIT_SUCCESS on success, or -1 if there was an error.
 */
int create_fil(dada_client_t *client, cFilFile *out_filfile_ptr)
{
  assert(client != 0);  

  assert(client->log != 0);
  multilog_t *log = (multilog_t *) client->log;
  dada_db_s* ctx = (dada_db_s*) client->context;
  multilog(log, LOG_INFO, "create_fil(): Creating new fil file %s...\n", ctx->fil_filename);  
  
  // Create a new blank fil file  
  if (CFilFile_Open(out_filfile_ptr, ctx->fil_filename) != EXIT_SUCCESS)
  {
    char error_text[30]="";
    multilog(log, LOG_ERR, "create_fil(): Error creating fil file: %s. Error: %s\n", ctx->fil_filename, error_text);
    return -1;
  }

  multilog(log, LOG_INFO, "create_fil(): here1.\n");  

  // Write header
  cFilFileHeader filheader;

  // Init header struct
  CFilFileHeader_Constructor(&filheader);

  // Populate header
  filheader.telescope_id = 0;                                               // FAKE
  filheader.machine_id = 0;                                                 // FAKE
  filheader.data_type = 2;                                                  // 1 - filterbank; 2 - timeseries
  strncpy(filheader.rawdatafile, ctx->fil_filename, 4096);
  strncpy(filheader.source_name, "None", 4096);  
  filheader.barycentric = 0;
  filheader.pulsarcentric = 0;  
  filheader.az_start = 0;                                                   // Pointing azimuth (degrees)
  filheader.za_start = 0;                                                   // Pointing zenith angle (degrees)
  filheader.src_raj = 0;                                                    // RA (J2000) of source
  filheader.src_dej = 0;                                                    // DEC (J2000) of source
  filheader.tstart = 0;                                                     // Timestamp MJD of first sample
  filheader.tsamp = ctx->ntimes / 1000.0;                                   // time interval between samples (seconds)
  filheader.nbits = ctx->nbit;                                              // bits per time sample
  filheader.fch1 = 0;                                                       // Centre freq (MHz) of first channel
  filheader.foff = 1.28;                                                    // filterbank channel bandwidth (MHz)  
  
  // This is an array of channels (in MHz)
  filheader.fchannel = 0;   

  filheader.nchans = ctx->nfine_chan;
  filheader.nifs = ctx->npol;                                               // Number of IF channels(polarisations I think)
  filheader.refdm = 0;                                                      // reference dispersion measure (cm^−3 pc)
  filheader.period = 0;                                                     // folding period (s)
  filheader.npuls = 0;        
  filheader.nbins = ctx->ntimes;
  
  strncpy(filheader.signed_, " ", 1);
  filheader.nsamples = ctx->ntimes * ctx->nfine_chan * ctx->npol * ctx->exposure_sec; // number of time samples in the data file (rarely used)
  filheader.nbeams = ctx->nbeams;
  filheader.ibeam = 0;

  // Write the header
  CFilFile_WriteHeader(out_filfile_ptr, &filheader);    

  return(EXIT_SUCCESS);
}

/**
 *
 *  @brief Closes the fil file.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in,out] fptr Pointer to a pointer to the filfile structure.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int close_fil(dada_client_t *client, cFilFile *out_filfile_ptr)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  multilog(log, LOG_DEBUG, "close_fil(): Starting.\n");  

  if (out_filfile_ptr != NULL)
  {
    if (CFilFile_Close(out_filfile_ptr) != EXIT_SUCCESS)
    {
      char error_text[30]="";      
      multilog(log, LOG_ERR, "close_fil(): Error closing fil file. Error: %s\n", error_text);
      return EXIT_FAILURE;
    }
    else
    {
      out_filfile_ptr = NULL;
    }
  }
  else
  {
    multilog(log, LOG_WARNING, "close_fil(): fil file is already closed.\n");
  }

  return(EXIT_SUCCESS);
}

/**
 *
 *  @brief Creates a new block in an existing fil file.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] fptr Pointer to the fil file we will write to.
 *  @param[in] fine_channels The number of fine channels.
 *  @param[in] polarisations The number of pols in each beam.
 *  @param[in] buffer The pointer to the data to write into the block.
 *  @param[in] bytes The number of bytes in the buffer to write.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int create_fil_block(dada_client_t *client, cFilFile *out_filfile_ptr, int fine_channels, int polarisations, float *buffer, uint64_t bytes)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  // write stuff
  int out_floats = CFilFile_WriteData(out_filfile_ptr, buffer, fine_channels * polarisations);

  if (out_floats*sizeof(float) != bytes)
  {
    // Error
    char error_text[30]="";      
    multilog(log, LOG_ERR, "create_fil_block(): Error writing fil file block. Number of bytes %d != %d floats * sizeof(float) Error: %s\n", bytes, out_floats * sizeof(float),error_text);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}