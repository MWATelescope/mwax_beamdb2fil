/**
 * @file filwriter.c
 * @author Greg Sleap
 * @date 15 June 2019
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
int create_fil(dada_client_t *client, int beam_index, cFilFile *out_filfile_ptr, metafits_s *metafits)
{
  assert(client != 0);  

  assert(client->log != 0);
  multilog_t *log = (multilog_t *) client->log;
  dada_db_s* ctx = (dada_db_s*) client->context;

  beam_s beam = ctx->beams[beam_index];

  multilog(log, LOG_INFO, "create_fil(): Creating new fil file for beam %d: %s...\n", beam_index, beam.fil_filename);  
  
  // Create a new blank fil file  
  if (CFilFile_Open(out_filfile_ptr, beam.fil_filename) != EXIT_SUCCESS)
  {
    char error_text[30]="";
    multilog(log, LOG_ERR, "create_fil(): Error creating fil file: %s. Error: %s\n", beam.fil_filename, error_text);
    return -1;
  }  

  // Write header
  cFilFileHeader filheader;

  // Init header struct
  CFilFileHeader_Constructor(&filheader);

  // Populate header
  filheader.telescope_id = 0;                                                     // FAKE
  filheader.machine_id = 0;                                                       // FAKE
  filheader.data_type = 1;                                                        // 1 - filterbank; 2 - timeseries
  strncpy(filheader.rawdatafile, beam.fil_filename, 4096);
  strncpy(filheader.source_name, metafits->filename, 4096);  
  filheader.barycentric = 0;
  filheader.pulsarcentric = 0;  
  filheader.az_start = metafits->azimuth;                                         // Pointing azimuth (degrees)
  filheader.za_start = 90 - metafits->altitude;                                   // Pointing zenith angle (degrees)
  filheader.src_raj = metafits->ra;                                               // RA (J2000) of source
  filheader.src_dej = metafits->dec;                                              // DEC (J2000) of source
  filheader.tstart = metafits->mjd;                                               // Timestamp MJD of first sample
  filheader.tsamp = 1.0f / beam.ntimesteps;                                       // time interval between samples (seconds)
  filheader.nbits = ctx->nbit;                                                    // bits per time sample  
  filheader.nsamples = beam.ntimesteps * ctx->exposure_sec;                       // number of time samples in the data file (rarely used)
  filheader.fch1 = beam.channels[beam.nchan - 1];                                 // Centre freq (MHz) of last channel  
  filheader.foff = -(double)ctx->bandwidth_hz / 1000000.0f / (double)beam.nchan;  // fine channel bandwidth (MHz) - negative since we provide higest freq in fch1
  filheader.nchans = beam.nchan;  
  filheader.nifs = ctx->npol;                                                     // Number of IF channels(polarisations I think)
  filheader.refdm = 0;                                                            // reference dispersion measure (cm^−3 pc)
  filheader.period = 0.253065;                                                    // folding period (s)
  filheader.nbeams = 1;
  filheader.ibeam = 0;

  multilog(log, LOG_INFO, "create_fil(): filheader.tsamp   : %f sec per sample\n", filheader.tsamp);
  multilog(log, LOG_INFO, "create_fil(): filheader.nsamples: %ld total samples (timesteps per sec %ld * duration %d sec)\n", filheader.nsamples, beam.ntimesteps, ctx->exposure_sec);
  multilog(log, LOG_INFO, "create_fil(): filheader.fch1    : %f MHz (center of last) channel\n", filheader.fch1);
  multilog(log, LOG_INFO, "create_fil(): filheader.foff    : %f MHz width of fine channel (negative since we start at highest)\n", filheader.foff);
  multilog(log, LOG_INFO, "create_fil(): filheader.nchans  : %ld number of channels\n", filheader.nchans);
  
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
 *  @param[in] bytes_per_sample The number of bytes per sample.
 *  @param[in] timesteps The number of timesteps to write.
 *  @param[in] fine_channels The number of fine channels.
 *  @param[in] polarisations The number of pols in each beam.
 *  @param[in] buffer The pointer to the data to write into the block.
 *  @param[in] bytes The number of bytes in the buffer to write.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int create_fil_block(dada_client_t *client, cFilFile *out_filfile_ptr, int bytes_per_sample, long timesteps, 
                     long fine_channels, int polarisations, float *buffer, uint64_t bytes)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) ctx->log;

  // write stuff
  uint64_t buffer_elements = timesteps * fine_channels * polarisations;
  uint64_t in_check_bytes = buffer_elements * bytes_per_sample;

  if (in_check_bytes != bytes)
  {
    // Error
    char error_text[30]="";      
    multilog(log, LOG_ERR, "create_fil_block(): Error writing fil file block. Number of bytes %" PRIu64 " != %" PRIu64 " (samples * bytes per sample)-> (t: %d, f: %d p: %d b: %d) Error: %s\n", bytes, in_check_bytes, timesteps, fine_channels, polarisations, bytes_per_sample, error_text);
    return EXIT_FAILURE;
  }

  int out_samples = CFilFile_WriteData(out_filfile_ptr, buffer, buffer_elements);
  uint64_t out_check_bytes = out_samples * bytes_per_sample;

  if (out_check_bytes != bytes)
  {
    // Error
    char error_text[30]="";      
    multilog(log, LOG_ERR, "create_fil_block(): Error writing fil file block. Number of bytes written%" PRIu64 " != %" PRIu64 " (samples * bytes per sample) Error: %s\n", bytes, out_check_bytes, error_text);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}