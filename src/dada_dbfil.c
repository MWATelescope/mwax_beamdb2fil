/**
 * @file dada_dbfil.c
 * @author Greg Sleap
 * @date 23 May 2018
 * @brief This is the code that drives the ring buffers
 *
 */
#include <stdio.h>
#include <errno.h>
#include "dada_dbfil.h"
#include "mwax_global_defs.h" // From mwax-common

/**
 * 
 *  @brief This is called at the begininning of each new 8 second sub-observation.
 *         We need check if we are in a new fils file or continuing the existing one.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @returns EXIT_SUCCESS on success, or -1 if there was an error. 
 */
int dada_dbfil_open(dada_client_t* client)
{
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t *log = (multilog_t *) client->log;  
  multilog(log, LOG_INFO, "dada_dbfil_open(): extracting params from dada header\n");

  // These need to be set for psrdada
  client->transfer_bytes = 0;  
  client->optimal_bytes = 0;

  // we do not want to explicitly transfer the DADA header
  client->header_transfer = 0;

  // Read the command first
  strncpy(ctx->command, "", MWAX_COMMAND_LEN);  
  if (ascii_header_get(client->header, HEADER_COMMAND, "%s", &ctx->command) == -1)
  {
    multilog(log, LOG_ERR, "dada_dbfil_open(): %s not found in header.\n", HEADER_COMMAND);
    return -1;
  }

  // Verify command is ok
  if (strlen(ctx->command) > 0)
  {
    multilog(log, LOG_INFO, "dada_dbfil_open(): %s == %s\n", HEADER_COMMAND, ctx->command);

    if (strncmp(ctx->command, MWAX_COMMAND_CAPTURE, MWAX_COMMAND_LEN) == 0)
    {
      // Normal operations      
    }  
    else if (strncmp(ctx->command, MWAX_COMMAND_QUIT, MWAX_COMMAND_LEN) == 0)
    {
      // We'll flag we want to quit
      set_quit(1);
      return EXIT_SUCCESS;
    }
    else if(strncmp(ctx->command, MWAX_COMMAND_IDLE, MWAX_COMMAND_LEN) == 0)
    {
      // Idle- don't produce files 
      return EXIT_SUCCESS;
    }
    else
    {
      // Invalid command
      multilog(log, LOG_ERR, "dada_dbfil_open(): Error: %s '%s' not recognised.\n", HEADER_COMMAND, ctx->command);
      return -1;
    }
  }
  else
  {
    // No command provided at all! 
    multilog(log, LOG_ERR, "dada_dbfil_open(): Error: an empty %s was provided.\n", HEADER_COMMAND);
    return -1;
  }

  // get the obs_id of this subobservation
  long this_obs_id = 0;
  if (ascii_header_get(client->header, HEADER_OBS_ID, "%lu", &this_obs_id) == -1)
  {
    multilog(log, LOG_ERR, "dada_dbfil_open(): %s not found in header.\n", HEADER_OBS_ID);
    return -1;
  }

  long this_subobs_id = 0;
  if (ascii_header_get(client->header, HEADER_SUBOBS_ID, "%lu", &this_subobs_id) == -1)
  {
    multilog(log, LOG_ERR, "dada_dbfil_open(): %s not found in header.\n", HEADER_SUBOBS_ID);
    return -1;
  }

  // Sanity check this obs_id
  if (!(this_obs_id > 0))
  {
    multilog(log, LOG_ERR, "dada_dbfil_open(): New %s is not greater than 0.\n", HEADER_OBS_ID);
    return -1;
  }

  // Check this obs_id against our 'in progress' obsid  
  if (ctx->obs_id != this_obs_id || ctx->bytes_written >= FIL_SIZE_CUTOFF_BYTES)
  {
    // We need a new fits file
    if (ctx->obs_id != this_obs_id)
    {
      if (&(ctx->out_filfile_ptr) != NULL)
      {
        multilog(log, LOG_INFO, "dada_dbfil_open(): New %s detected. Closing %lu, Starting %lu...\n", HEADER_OBS_ID, ctx->obs_id, this_obs_id);
      }
      else
      {
        multilog(log, LOG_INFO, "dada_dbfil_open(): New %s detected. Starting %lu...\n", HEADER_OBS_ID, this_obs_id);
      }      
    }
    else
    {
      multilog(log, LOG_INFO, "dada_dbfil_open(): Exceeded max size %lu of a fits file. Closing %s, Starting new file...\n", FIL_SIZE_CUTOFF_BYTES, ctx->fil_filename);
    }

    // Close existing fits file (if we have one)    
    if (&(ctx->out_filfile_ptr) != NULL)
    {
      if (close_fil(client, &(ctx->out_filfile_ptr))) 
      {
        multilog(log, LOG_ERR,"dada_dbfil_open(): Error closing fits file.\n");
        return -1;
      }
    }
    
    if (ctx->obs_id != this_obs_id)
    {
      //
      // Do this for new observations only
      //

      // initialise our structure
      ctx->block_open = 0;
      ctx->bytes_read = 0;
      ctx->bytes_written = 0;
      ctx->curr_block = 0;
      ctx->block_number= 0;

      // fil info  
      strncpy(ctx->fil_filename, "", PATH_MAX);      

      // Set the obsid & sub obsid 
      ctx->obs_id = this_obs_id;
      ctx->subobs_id = this_subobs_id;

      // Read in all of the info from the header into our struct
      if (read_dada_header(client))
      {
        // Error processing in header!
        multilog(log, LOG_ERR,"dada_dbfil_open(): Error processing header.\n");
        return -1;
      }      

      /*                          */
      /* Sanity check what we got */
      /*                          */            
      /* unix time msec must be between 0 and 999 
      if (!(ctx->unix_time_msec >= 0 || ctx->unix_time_msec<1000))
      {
        multilog(log, LOG_ERR, "dada_dbfil_open(): %s must be between 0 and 999 milliseconds.\n", HEADER_UNIXTIME_MSEC);
        return -1;
      } */
      
      //
      // Check transfer size read in from header matches what we expect from the other params      
      //     
      // one sub obs = beams * pols * timesteps per chan * chans * size of a sample
      //             
      // The number of bytes should never exceed transfer size
      if (ctx->expected_transfer_size > ctx->transfer_size)
      {
        multilog(log, LOG_ERR, "dada_dbfil_open(): %s provided in header (%lu bytes) is not large enough for a subobservation size of (%lu bytes).\n", HEADER_TRANSFER_SIZE, ctx->transfer_size, ctx->expected_transfer_size);
        return -1; 
      }

      // Also confirm that the integration size can fit into the ringbuffer size
      if (ctx->expected_transfer_size > ctx->block_size)
      {
        multilog(log, LOG_ERR, "dada_dbfil_open(): Ring buffer block size (%lu bytes) is less than the calculated size of an integration from header parameters (%lu bytes).\n", ctx->block_size, ctx->expected_transfer_size);
        return -1;
      }
      
      // Reset the filenumber
      ctx->fil_file_number = 0;      
    }
    else
    {
      // Do this only if we exceeded the size of a fits file and need a new file
      ctx->fil_file_number++;
    }

    /* Create fil file for output                                */
    /* Work out the name of the file using the UTC START          */
    /* Convert the UTC_START from the header format: YYYY-MM-DD-hh:mm:ss into YYYYMMDDhhmmss  */        
    int year, month, day, hour, minute, second;
    sscanf(ctx->utc_start, "%d-%d-%d-%d:%d:%d", &year, &month, &day, &hour, &minute, &second);    
      
    /* Make a new filename- oooooooooo_YYYYMMDDhhmmss_chCCC_FFF.fil */
    snprintf(ctx->fil_filename, PATH_MAX, "%s/%ld_%04d%02d%02d%02d%02d%02d_ch%02d_%03d.fil", ctx->destination_dir, ctx->obs_id, year, month, day, hour, minute, second, ctx->coarse_channel, ctx->fil_file_number);
    
    if (create_fil(client, &(ctx->out_filfile_ptr))) 
    {
      multilog(log, LOG_ERR,"dada_dbfil_open(): Error creating new fil file.\n");
      return -1;
    }
  }
  else
  {
    /* This is a continuation of an existing observation */
    multilog(log, LOG_INFO, "dada_dbfil_open(): continuing %lu...\n", ctx->obs_id);
  }
    
  multilog(log, LOG_INFO, "dada_dbfil_open(): completed\n");

  return EXIT_SUCCESS;
}

/**
 * 
 *  @brief This is the function psrdada calls when we have new data to read.
 *         NOTE: this method reads an entire block of PSRDADA ringbuffer. 
 * 
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] buffer The pointer to the data in the ringbuffer we are about to read.
 *  @param[in] bytes The number of bytes that are available to be read from the ringbuffer.
 *  @returns the number of bytes read or -1 if there was an error.
 */
int64_t dada_dbfil_io(dada_client_t *client, void *buffer, uint64_t bytes)
{
  assert (client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  if (strcmp(ctx->command, MWAX_COMMAND_CAPTURE) == 0)
  {
    multilog_t * log = (multilog_t *) ctx->log;
    
    uint64_t written  = 0;    
    uint64_t wrote    = 0;
    
    multilog (log, LOG_DEBUG, "dada_dbfil_io(): Processing block %d.\n", ctx->block_number);
      
    multilog(log, LOG_INFO, "dada_dbfil_io(): Writing %d of %d bytes into new fil block; Marker = %d.\n", ctx->expected_transfer_size, bytes, ctx->obs_marker_number);     
    
    // Read ring buffer block and write data out    
    float* in_buffer = (float*)buffer;
    int out_buffer_elements = 0;

    for (int beam=0; beam < ctx->nbeams; beam++)
    {
      out_buffer_elements += ctx->beams[beam].ntimesteps * ctx->beams[beam].nchan * ctx->npol;
    }

    int out_buffer_bytes = out_buffer_elements * sizeof(float);    
                    
    int input_index = 0;

    // Loop through each beam
    for (int beam=0; beam < ctx->nbeams; beam++)
    {      
      ctx->beams[beam].power_freq = calloc(ctx->beams[beam].nchan, sizeof(double));
      ctx->beams[beam].power_time = calloc(ctx->beams[beam].ntimesteps, sizeof(double));

      // For each beam loop through all of the timesteps
      for (long t=0;t<ctx->beams[beam].ntimesteps;t++)
      {
          // Iterate through all of the channels in the timestep
          for (int ch=0; ch<ctx->beams[beam].nchan; ch++)
          {
              // Each channel can have polarisations
              for (int pol=0; pol<ctx->npol; pol++)
              {
                //int input_index = (t * ctx->nfine_chan * ctx->npol) + (ch * ctx->npol) + pol;
                
                // Update stats but only if we asked for it
                if (ctx->stats_dir != NULL)
                {
                  ctx->beams[beam].power_freq[ch] += (double)in_buffer[input_index];              
                  ctx->beams[beam].power_time[t] += (double)in_buffer[input_index];              
                }

                /* uncomment this for debug! 
                if (t<=0 && ctx->block_number==0)
                  printf("t=%d; ch=%d; pol=%d; in_index=%d; out_index=%d value=%f;\n", t, ch, pol, input_index, output_index, in_buffer[input_index]);
                */

               // increment the input_data index
               input_index = input_index + 1;
              }            
          }        
      }

      // Create the fil block for this beam        
      if (create_fil_block(client, &(ctx->out_filfile_ptr), ctx->nbit/8, ctx->beams[beam].ntimesteps, ctx->beams[beam].nchan, ctx->npol, (float*)buffer, out_buffer_bytes))    
      {
        // Error!
        multilog(log, LOG_ERR, "dada_dbfil_io(): Error Writing into new fil block.\n");
        return -1;
      }
      else
      {   
        wrote = out_buffer_bytes;
        written += wrote;
        ctx->obs_marker_number += 1; // Increment the marker number

        ctx->block_number += 1;    
        ctx->bytes_written += written;

        if (ctx->stats_dir != NULL)
        {
          /* Make a new filename for the freq stats */
          char output_spectrum_filename[PATH_MAX];
          snprintf(output_spectrum_filename, PATH_MAX, "%s/%ld_ch%02d_%02d_%03d_spec.txt", 
                  ctx->stats_dir, ctx->obs_id, ctx->coarse_channel, beam + 1, ctx->block_number);

          FILE* out_fs = fopen(output_spectrum_filename, "w");
          for(int ch=0;ch<ctx->beams[beam].nchan;ch++)
          {
              ctx->beams[beam].power_freq[ch] = ctx->beams[beam].power_freq[ch] / (double)ctx->beams[beam].ntimesteps;
              
              fprintf(out_fs,"%d %f\n",ch, ctx->beams[beam].power_freq[ch]);          
          }
          fclose(out_fs);

          /* Make a new filename for the time stats */
          char output_time_filename[PATH_MAX];
          snprintf(output_time_filename, PATH_MAX, "%s/%ld_ch%02d_%02d_%03d_time.txt", 
                  ctx->stats_dir, ctx->obs_id, ctx->coarse_channel, beam + 1, ctx->block_number);

          FILE* out_ft = fopen(output_time_filename, "w");
          for(long t=0;t<ctx->beams[beam].ntimesteps;t++)
          {
              ctx->beams[beam].power_time[t] = ctx->beams[beam].power_time[t] / (double)ctx->beams[beam].nchan;
              
              fprintf(out_ft,"%ld %f\n",t, ctx->beams[beam].power_time[t]);          
          }
          fclose(out_ft);

          multilog(log, LOG_INFO, "dada_dbfil_io(): wrote out spectrum (%s) and time (%s) statistics.", output_spectrum_filename, output_time_filename);
        }
      }                
    
      // Cleanup    
      free(ctx->beams[beam].power_freq);
      free(ctx->beams[beam].power_time);
    }

    return bytes;
  }  
  else
    return 0;
}

/**
 * 
 *  @brief This is called when we are reading a sub block of an 8 second sub-observation.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] buffer The pointer to the data in the ringbuffer we are about to read.
 *  @param[in] bytes The number of bytes that are available to be read from the ringbuffer
 *  @param[in] block_id The block number (id) of the block we are reading.
 *  @returns the number of bytes read, or -1 if there was an error.
 */
int64_t dada_dbfil_io_block(dada_client_t *client, void *buffer, uint64_t bytes, uint64_t block_id)
{
  assert (client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  if (strcmp(ctx->command, MWAX_COMMAND_CAPTURE) == 0)
  {
    multilog_t * log = (multilog_t *) ctx->log;

    multilog(log, LOG_INFO, "dada_dbfil_io_block(): Processing block id %llu\n", block_id);

    return dada_dbfil_io(client, buffer, bytes);
  }
  else    
    return bytes;
}

/**
 * 
 *  @brief This is called at the end of each new 8 second sub-observation.
 *  @param[in] client A pointer to the dada_client_t object.
 *  @param[in] bytes_written The number of bytes that psrdada has written for this entire 8 second subobservation.
 *  @returns EXIT_SUCCESS on success, or -1 if there was an error.
 */
int dada_dbfil_close(dada_client_t* client, uint64_t bytes_written)
{
  assert (client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  multilog_t *log = (multilog_t *) client->log;
  multilog(log, LOG_INFO, "dada_dbfil_close(bytes_written=%lu): Started.\n", bytes_written);
  
  int do_close_file = 0;

  // If we're still in CAPTURE mode...
  if (strcmp(ctx->command, MWAX_COMMAND_CAPTURE) == 0)
  {
    // Some sanity checks:
    // Did we hit the end of an obs
    if (1 == 1)
    {      
      do_close_file = 1;
    }
  }
  else if (strcmp(ctx->command, MWAX_COMMAND_QUIT) == 0 || strcmp(ctx->command, MWAX_COMMAND_IDLE) == 0)
  {
    do_close_file = 1;
  }

  if (do_close_file == 1)
  {
    // Observation ends NOW! It got cut short, or we naturally are at the end of the observation 
    // Close existing fits file (if we have one)    
    if (&(ctx->out_filfile_ptr) != NULL)
    {
      if (close_fil(client, &ctx->out_filfile_ptr)) 
      {
        multilog(log, LOG_ERR,"dada_dbfil_close(): Error closing fil file.\n");
        return -1;
      }
    }
  }   

  multilog (log, LOG_INFO, "dada_dbfil_close(): completed\n");

  return EXIT_SUCCESS;
}

/**
 * 
 *  @brief This reads a PSRDADA header and populates our context structure and dumps the contents into a debug log
 *  @param[in] client A pointer to the dada_client_t object. 
 *  @returns EXIT_SUCCESS on success, or -1 if there was an error.
 */
int read_dada_header(dada_client_t *client)
{
  // Reset and read everything except for obs_id and subobs_id
  assert(client != 0);
  dada_db_s* ctx = (dada_db_s*) client->context;

  assert(ctx->log != 0);
  multilog_t * log = (multilog_t *) ctx->log;

  strncpy(ctx->utc_start, "", UTC_START_LEN);
  ctx->nbit = 0;
  ctx->npol = 0;
  ctx->nbeams = 0;
  ctx->transfer_size = 0;
  ctx->coarse_channel = 0;
    
  if (ascii_header_get(client->header, HEADER_UTC_START, "%s", &ctx->utc_start) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_UTC_START);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_NBIT, "%i", &ctx->nbit) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_NBIT);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_NPOL, "%i", &ctx->npol) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_NPOL);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_TRANSFER_SIZE, "%lu", &ctx->transfer_size) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_TRANSFER_SIZE);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_COARSE_CHANNEL, "%i", &ctx->coarse_channel) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_COARSE_CHANNEL);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_EXPOSURE_SECS, "%i", &ctx->exposure_sec) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_EXPOSURE_SECS);
    return -1;
  }  
  
  if (ascii_header_get(client->header, HEADER_METADATA_BEAMS, "%i", &ctx->nbeams) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_METADATA_BEAMS);
    return -1;
  }  

  // Process Beams
  if (ctx->beams != 0)
    free(ctx->beams);
  
  // Allocate beams
  ctx->beams = malloc(ctx->nbeams * sizeof(beam_s));
  
  int beam_inttime_string_len = strlen(HEADER_INCOHERENT_BEAM_01_TIME_INTEG)+1;
  char beam_inttime_string[beam_inttime_string_len];  

  int beam_finechan_string_len = strlen(HEADER_INCOHERENT_BEAM_01_CHANNELS)+1;
  char beam_finechan_string[beam_finechan_string_len];

  ctx->expected_transfer_size = 0;

  for (int beam_index = 0; beam_index < ctx->nbeams; beam_index++)
  {            
    int beam = beam_index + 1;

    if (beam == 1)
    {
      strncpy(beam_inttime_string, HEADER_INCOHERENT_BEAM_01_TIME_INTEG, beam_inttime_string_len);
      strncpy(beam_finechan_string, HEADER_INCOHERENT_BEAM_01_CHANNELS, beam_finechan_string_len);
    }
    else
    {
      multilog(log, LOG_ERR, "read_dada_header(): %d or more beams are not supported.\n", beam);
      return -1;
    }
        
    if (ascii_header_get(client->header, beam_inttime_string, "%ld", &ctx->beams[beam_index].time_integration) == -1)
    {
      multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", beam_inttime_string);
      return -1;
    }  

    // Now we can calculate ntimesteps for this beam
    // unchannelised samples/sec divided by time_integration
    // e.g. time_integration of 1280 will yield 1280000 / 1280 == 1000 timesteps per sec
    ctx->beams[beam_index].ntimesteps = (long)1280000 / ctx->beams[beam_index].time_integration;    

    if (ascii_header_get(client->header, beam_finechan_string, "%ld", &ctx->beams[beam_index].nchan) == -1)
    {
      multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", beam_finechan_string);
      return -1;
    }  

    ctx->expected_transfer_size = ctx->expected_transfer_size + (ctx->beams[beam_index].ntimesteps * ctx->beams[beam_index].nchan * ctx->npol * (ctx->nbit/8));
  }

  // Output what we found in the header
  multilog(log, LOG_INFO, "Obs Id:                   %lu\n", ctx->obs_id);
  multilog(log, LOG_INFO, "Subobs Id:                %lu\n", ctx->subobs_id);
  multilog(log, LOG_INFO, "Command:                  %s\n", ctx->command);  
  multilog(log, LOG_INFO, "Start time (UTC):         %s\n", ctx->utc_start);  
  multilog(log, LOG_INFO, "Duration (secs):          %d\n", ctx->exposure_sec);    
  multilog(log, LOG_INFO, "Bits per real/imag:       %d\n", ctx->nbit);  
  multilog(log, LOG_INFO, "Polarisations:            %d\n", ctx->npol);
  multilog(log, LOG_INFO, "Coarse channel no.:       %d\n", ctx->coarse_channel);  
  multilog(log, LOG_INFO, "Size of subobservation:   %lu bytes\n", ctx->transfer_size);
  multilog(log, LOG_INFO, "Beams:                    %d\n", ctx->nbeams);    

  for (int beam=0; beam < ctx->nbeams; beam++)
  {
    multilog(log, LOG_INFO, "..Beam %.2d channels:            %d\n", beam+1, ctx->beams[beam].nchan);
    multilog(log, LOG_INFO, "..Beam %.2d time int (tscrunch): %ld\n", beam+1, ctx->beams[beam].time_integration);
    multilog(log, LOG_INFO, "..Beam %.2d timesteps/sec:       %ld\n", beam+1, ctx->beams[beam].ntimesteps);    
  }
  //multilog(log, LOG_INFO, "No of chans per timestep: %d\n", ctx->nfine_chan);
  //multilog(log, LOG_INFO, "Timesteps:                %d\n", ctx->ntimesteps);  
  
  return EXIT_SUCCESS;
}
