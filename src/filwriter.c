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
#include "util.h"

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
  multilog_t *log = (multilog_t *)client->log;
  dada_db_s *ctx = (dada_db_s *)client->context;

  beam_s beam = ctx->beams[beam_index];

  multilog(log, LOG_INFO, "create_fil(): Creating new fil file for beam %d: %s...\n", beam_index, beam.fil_filename);

  // Create a new blank fil file
  if (CFilFile_Open(out_filfile_ptr, beam.fil_filename) != EXIT_SUCCESS)
  {
    char error_text[30] = "";
    multilog(log, LOG_ERR, "create_fil(): Error creating fil file: %s. Error: %s\n", beam.fil_filename, error_text);
    return -1;
  }

  // Write header
  cFilFileHeader filheader;

  // Init header struct
  CFilFileHeader_Constructor(&filheader);

  // Populate header
  int d, h, m;
  double s;

  // Convert to hms
  degrees_to_hms(metafits->ra, &h, &m, &s);

  // Reformat into hhmmss.s
  double ra = format_angle(h, m, s);

  // Convert to dms
  degrees_to_dms(metafits->dec, &d, &m, &s);

  // Reformat into ddmmss.s
  double dec = format_angle(d, m, s);

  // Other fields
  filheader.telescope_id = 0; // FAKE
  filheader.machine_id = 0;   // FAKE
  filheader.data_type = 1;    // 1 - filterbank; 2 - timeseries
  strncpy(filheader.rawdatafile, beam.fil_filename, 4096);
  strncpy(filheader.source_name, metafits->filename, 4095);
  filheader.barycentric = 0;
  filheader.pulsarcentric = 0;
  filheader.az_start = metafits->azimuth;                                               // Pointing azimuth (degrees)
  filheader.za_start = 90 - metafits->altitude;                                         // Pointing zenith angle (degrees)
  filheader.src_raj = ra;                                                               // RA (J2000) of source hhmmss.s
  filheader.src_dej = dec;                                                              // DEC (J2000) of source ddmmss.s
  filheader.tstart = metafits->mjd;                                                     // Timestamp MJD of first sample
  filheader.tsamp = 1.0f / beam.ntimesteps;                                             // time interval between samples (seconds)
  filheader.nbits = ctx->nbit;                                                          // bits per time sample
  filheader.nsamples = beam.ntimesteps * ctx->exposure_sec;                             // number of time samples in the data file (rarely used)
  filheader.fch1 = beam.channels[0];                                                    // Start freq (MHz) of first channel
  filheader.foff = (double)ctx->bandwidth_hz / (double)1000000.0f / (double)beam.nchan; // fine channel bandwidth (MHz) - negative since we provide higest freq in fch1
  filheader.nchans = beam.nchan;
  filheader.nifs = ctx->npol; // Number of IF channels(polarisations I think)
  filheader.refdm = 0;        // reference dispersion measure (cm^−3 pc)
  filheader.period = 0;       // folding period (s)
  filheader.nbeams = 1;       // Total beams in file
  filheader.ibeam = 1;        // Beam number

  multilog(log, LOG_INFO, "create_fil(): filheader.telescope_id : %d (0=FAKE)\n", filheader.telescope_id);
  multilog(log, LOG_INFO, "create_fil(): filheader.machine_id   : %d (0=FAKE)\n", filheader.machine_id);
  multilog(log, LOG_INFO, "create_fil(): filheader.data_type    : %d (1 - filterbank; 2 - timeseries)\n", filheader.data_type);
  multilog(log, LOG_INFO, "create_fil(): filheader.rawdatafile  : %s\n", filheader.rawdatafile);
  multilog(log, LOG_INFO, "create_fil(): filheader.source_name  : %s\n", filheader.source_name);
  multilog(log, LOG_INFO, "create_fil(): filheader.barycentric  : %d\n", filheader.barycentric);
  multilog(log, LOG_INFO, "create_fil(): filheader.pulsarcentric: %d\n", filheader.pulsarcentric);
  multilog(log, LOG_INFO, "create_fil(): filheader.az_start     : %f Pointing azimuth (degrees)\n", filheader.az_start);
  multilog(log, LOG_INFO, "create_fil(): filheader.za_start     : %f Pointing zenith angle (degrees)\n", filheader.za_start);
  multilog(log, LOG_INFO, "create_fil(): filheader.src_raj      : %f RA (J2000) of source\n", filheader.src_raj);
  multilog(log, LOG_INFO, "create_fil(): filheader.src_dej      : %f DEC (J2000) of source\n", filheader.src_dej);
  multilog(log, LOG_INFO, "create_fil(): filheader.tstart       : %f MJD of start\n", filheader.tstart);
  multilog(log, LOG_INFO, "create_fil(): filheader.tsamp        : %f sec per sample\n", filheader.tsamp);
  multilog(log, LOG_INFO, "create_fil(): filheader.nbits        : %d bits per sample\n", filheader.nbits);
  multilog(log, LOG_INFO, "create_fil(): filheader.nsamples     : %ld total samples (timesteps per sec %ld * duration %d sec)\n", filheader.nsamples, beam.ntimesteps, ctx->exposure_sec);
  multilog(log, LOG_INFO, "create_fil(): filheader.fch1         : %f MHz (start of first) channel\n", filheader.fch1);
  multilog(log, LOG_INFO, "create_fil(): filheader.foff         : %f MHz width of channel\n", filheader.foff);
  multilog(log, LOG_INFO, "create_fil(): filheader.nchans       : %ld number of channels\n", filheader.nchans);
  multilog(log, LOG_INFO, "create_fil(): filheader.nifs         : %d Number of pols?\n", filheader.nifs);
  multilog(log, LOG_INFO, "create_fil(): filheader.nbeams       : %d Number of beams\n", filheader.nbeams);
  multilog(log, LOG_INFO, "create_fil(): filheader.ibeam        : %d Beam number in this file\n", filheader.ibeam);

  // Write the header
  CFilFile_WriteHeader(out_filfile_ptr, &filheader);

  return (EXIT_SUCCESS);
}

/**
 *
 *  @brief Updates a filterbank file header value for a specific keyword (int only supported right now).
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] filfile_ptr pointer to FilFile which we are working on.
 *  @param[in] keyword string with the keyword to search for.
 *  @param[in] value new value for this keyword.
 *  @returns EXIT_SUCCESS always but will log WARNINGS if there are problems.
 */
int update_filfile_int(dada_client_t *client, cFilFile *filfile_ptr, char *keyword, int new_value)
{
  assert(client != 0);
  dada_db_s *ctx = (dada_db_s *)client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *)ctx->log;

  // Find the keyword for nsamples
  // 1. Start at top of file
  if (fseek(filfile_ptr->m_File, 0, SEEK_SET) == 0)
  {
    const size_t BUFF_SIZE = 4096; // This nsamples keyword should be in the first 4K of the file (the header is small)
    char buffer[BUFF_SIZE];

    // Read a big chunk of header
    size_t header_bytes_read = fread(buffer, 1, BUFF_SIZE, filfile_ptr->m_File);

    if (header_bytes_read == BUFF_SIZE)
    {
      multilog(log, LOG_DEBUG, "update_filfile_int(): read %ld bytes from header!\n", header_bytes_read);
      // Find "nsamples". ret will be the pointer to the start of that string
      int ret = binary_strstr(buffer, BUFF_SIZE, "nsamples", strlen("nsamples"));

      if (ret >= 0)
      {
        multilog(log, LOG_DEBUG, "update_filfile_int(): Found keyword nsamples in header!\n");

        // Determine offset in bytes to the first byte after the keyword. This should be the first byte of our signed int (4 bytes)
        int offset_bytes = ret + strlen(keyword);

        // Get the int value and check it is correct!
        // first get a copy of those 4 bytes
        int existing_value;
        memcpy(&existing_value, &buffer[offset_bytes], sizeof(int));
        multilog(log, LOG_DEBUG, "update_filfile_int(): existing value for %s in header is %d\n", keyword, existing_value);

        // fseek to the correct location to write the new value
        if (fseek(filfile_ptr->m_File, offset_bytes, SEEK_SET) == 0)
        {
          // Now overwrite the value with the new value. It will return 1 element written if successful - any other number failure
          size_t nsamples_ints_written = fwrite(&new_value, sizeof(int), 1, filfile_ptr->m_File);
          if (nsamples_ints_written != 1)
          {
            multilog(log, LOG_WARNING, "update_filfile_int(): Error Updating %s- Error writing new nsamples! Expected to write %d int, wrote %ld ints instead\n", keyword, 1, nsamples_ints_written);
          }
        }
        else
        {
          multilog(log, LOG_WARNING, "update_filfile_int(): Error Updating %s- could not fseek to the correct place to write new nsamples in header!\n", keyword);
        }
      }
      else
      {
        multilog(log, LOG_WARNING, "update_filfile_int(): Error Updating %s- could not find keyword nsamples in header! ret was %d\n", keyword, ret);
      }
    }
    else
    {
      multilog(log, LOG_WARNING, "update_filfile_int(): Error Updating %s- fread failed to read correct bytes. Expected %d, was %ld.\n", keyword, BUFF_SIZE, header_bytes_read);
    }
  }
  else
  {
    multilog(log, LOG_WARNING, "update_filfile_int(): Error Updating %s - fseek to start of file failed.\n", keyword);
  }

  // We should always return success for now
  return EXIT_SUCCESS;
}

/**
 *
 *  @brief Closes the fil file.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] beam_index The beam index/identifier.
 *  @param[in,out] fptr Pointer to a pointer to the filfile structure.
 *  @returns EXIT_SUCCESS on success, or EXIT_FAILURE if there was an error.
 */
int close_fil(dada_client_t *client, cFilFile *out_filfile_ptr, int beam_index)
{
  assert(client != 0);
  dada_db_s *ctx = (dada_db_s *)client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *)ctx->log;

  if (out_filfile_ptr != NULL)
  {
    // Close the filterbank file and ensure it's written out
    if (CFilFile_Close(out_filfile_ptr) != EXIT_SUCCESS)
    {
      char error_text[30] = "";
      multilog(log, LOG_ERR, "close_fil(): Error closing fil file. Error: %s\n", error_text);
      return EXIT_FAILURE;
    }
    out_filfile_ptr = NULL;

    // Check if the duration changed mid observation
    if (ctx->duration_changed == 1)
    {
      // Update the header (nsamples)
      int nsamples = ctx->beams[beam_index].ntimesteps * ctx->exposure_sec; // number of time samples in the data file (rarely used)

      multilog(log, LOG_INFO, "close_fil(): Beam: %d- Duration changed mid-observation, updating the header to update nsamples: %ld total samples (timesteps per sec %ld * duration %d sec)\n", nsamples, ctx->beams[beam_index].ntimesteps, ctx->exposure_sec);

      // Update the nsamples value
      // Reopen the filterbank file for rw
      out_filfile_ptr->m_File = fopen(out_filfile_ptr->m_szFileName, "r+");

      // Verify it is open
      CFilFile_CheckFile(out_filfile_ptr);

      update_filfile_int(client, out_filfile_ptr, "nsamples", nsamples);

      // Close it again
      if (CFilFile_Close(out_filfile_ptr) != EXIT_SUCCESS)
      {
        char error_text[30] = "";
        multilog(log, LOG_ERR, "close_fil(): Error closing fil file (after updating the nsamples value). Error: %s\n", error_text);
        return EXIT_FAILURE;
      }

      out_filfile_ptr = NULL;
    }
  }
  else
  {
    multilog(log, LOG_WARNING, "close_fil(): fil file is already closed.\n");
  }

  return EXIT_SUCCESS;
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
  dada_db_s *ctx = (dada_db_s *)client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *)ctx->log;

  // write stuff
  uint64_t buffer_elements = timesteps * fine_channels * polarisations;
  uint64_t in_check_bytes = buffer_elements * bytes_per_sample;

  if (in_check_bytes != bytes)
  {
    // Error
    char error_text[30] = "";
    multilog(log, LOG_ERR, "create_fil_block(): Error writing fil file block. Number of bytes %" PRIu64 " != %" PRIu64 " (samples * bytes per sample)-> (t: %d, f: %d p: %d b: %d) Error: %s\n", bytes, in_check_bytes, timesteps, fine_channels, polarisations, bytes_per_sample, error_text);
    return EXIT_FAILURE;
  }

  int out_samples = CFilFile_WriteData(out_filfile_ptr, buffer, buffer_elements);
  uint64_t out_check_bytes = out_samples * bytes_per_sample;

  if (out_check_bytes != bytes)
  {
    // Error
    char error_text[30] = "";
    multilog(log, LOG_ERR, "create_fil_block(): Error writing fil file block. Number of bytes written%" PRIu64 " != %" PRIu64 " (samples * bytes per sample) Error: %s\n", bytes, out_check_bytes, error_text);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
