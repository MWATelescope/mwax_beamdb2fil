/**
 * @file dada_dbfil.c
 * @author Greg Sleap
 * @date 23 May 2018
 * @brief This is the code that drives the ring buffers
 *
 */
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "global.h"
#include "dada_dbfil.h"
#include "ascii_header.h"
#include "filwriter.h"
#include "metafitsreader.h"
#include "mwax_global_defs.h" // From mwax-common

/**
 * 
 *  @brief This is called at the begininning of each new 8 second sub-observation.
 *         The PSRDADA header will tell us how many beams we have. Each beam is
 *         written to a seperate fil file, so this code sets that up.
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
  if (ctx->obs_id != this_obs_id)
  {
    // We need a new fil file
    if (&(ctx->beams) != NULL)
    {
      multilog(log, LOG_INFO, "dada_dbfil_open(): New %s detected. Closing %lu, Starting %lu...\n", HEADER_OBS_ID, ctx->obs_id, this_obs_id);
    }
    else
    {
      multilog(log, LOG_INFO, "dada_dbfil_open(): New %s detected. Starting %lu...\n", HEADER_OBS_ID, this_obs_id);
    }      

    // Close existing fil files (if we have any)    
    for (int beam=0; beam < ctx->nbeams_total; beam++)
    {
      if (&(ctx->beams[beam].out_filfile_ptr) != NULL)
      {
        multilog(log, LOG_INFO, "dada_dbfil_open(): Closing %s...\n", ctx->beams[beam].fil_filename);

        if (close_fil(client, &(ctx->beams[beam].out_filfile_ptr))) 
        {
          multilog(log, LOG_ERR,"dada_dbfil_open(): Error closing fils file.\n");
          return -1;
        }
      }
    }

    //
    // Do this for new observations only
    //

    // initialise our structure
    ctx->block_open = 0;
    ctx->bytes_read = 0;
    ctx->bytes_written = 0;
    ctx->curr_block = 0;
    ctx->block_number= 0;    

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

    // Open and Read metafits file            
    snprintf(ctx->metafits_filename, PATH_MAX, "%s/%ld.metafits", ctx->metafits_path, ctx->obs_id);

    multilog(log, LOG_INFO,"dada_dbfil_open(): Reading metafits file: %s\n", ctx->metafits_filename);

    if (open_fits(client, &ctx->in_metafits_ptr, ctx->metafits_filename) != EXIT_SUCCESS)
    {
      // Error!        
      exit(EXIT_FAILURE);
    }

    // Read data from metafits
    if (ctx->metafits_info != 0)
      free(ctx->metafits_info);

    ctx->metafits_info = malloc(sizeof(metafits_s));

    if (read_metafits(client, ctx->in_metafits_ptr, ctx->metafits_info) != EXIT_SUCCESS)
    {
      // Error!        
      return EXIT_FAILURE;
    }

    // Close metafits
    if (close_fits(client, &ctx->in_metafits_ptr) != EXIT_SUCCESS)
    {
      // Error!        
      return EXIT_FAILURE;
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
    /*
    if (ctx->expected_transfer_size > ctx->block_size)
    {
      multilog(log, LOG_ERR, "dada_dbfil_open(): Ring buffer block size (%lu bytes) is less than the calculated size of an integration from header parameters (%lu bytes).\n", ctx->block_size, ctx->expected_transfer_size);
      return -1;
    } */       

    /* Create fil files for each beam output                      */
    for (int beam=0; beam < ctx->nbeams_total; beam++)
    {
      /* Work out the name of the file using the UTC START          */
      /* Convert the UTC_START from the header format: YYYY-MM-DD-hh:mm:ss into YYYYMMDDhhmmss  */        
      int year, month, day, hour, minute, second;
      sscanf(ctx->utc_start, "%d-%d-%d-%d:%d:%d", &year, &month, &day, &hour, &minute, &second);    
        
      /* Make a new filename- oooooooooo_YYYYMMDDhhmmss_chCCC_FFF.fil */
      snprintf(ctx->beams[beam].fil_filename, PATH_MAX, "%s/%ld_%04d%02d%02d%02d%02d%02d_ch%02d_%02d.fil", ctx->destination_dir, 
               ctx->obs_id, year, month, day, hour, minute, second, ctx->coarse_channel, beam + 1);
      
      if (create_fil(client, beam, &(ctx->beams[beam].out_filfile_ptr), ctx->metafits_info)) 
      {
        multilog(log, LOG_ERR,"dada_dbfil_open(): Error creating new fil file for beam %d.\n", beam + 1);
        return -1;
      }
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
    
    // Determine which beam this is. For example with 3 beams:
    // Block 0 == 1st beam timestep 1
    // Block 1 == 2nd beam timestep 1
    // Block 2 == 3rd beam timestep 1
    // Block 3 == 1st beam timestep 2
    // Block 4 == 2nd beam timestep 2
    // Block 5 == 3rd beam timestep 2
    int beam = ctx->block_number % ctx->nbeams_total;

    multilog(log, LOG_INFO, "dada_dbfil_io(): Writing %d of %d bytes into new fil block for beam %d; Marker = %d.\n", ctx->expected_transfer_size, bytes, beam, ctx->obs_marker_number);     
    
    // Read ring buffer block and write data out    
    float* in_buffer = (float*)buffer;
    long out_buffer_elements = 0;
    
    out_buffer_elements = ctx->beams[beam].ntimesteps * ctx->beams[beam].nchan * ctx->npol;
    
    long out_buffer_bytes = out_buffer_elements * sizeof(float);    
                    
    int input_index = 0;    

    ctx->beams[beam].power_freq = calloc(ctx->beams[beam].nchan, sizeof(double));
    ctx->beams[beam].power_time = calloc(ctx->beams[beam].ntimesteps, sizeof(double));

    // For this beam loop through all of the timesteps
    for (long t=0;t<ctx->beams[beam].ntimesteps;t++)
    {
        // Iterate through all of the channels in the timestep
        for (long ch=0; ch<ctx->beams[beam].nchan; ch++)
        {                                        
            // Each channel can have polarisations
            for (int pol=0; pol<ctx->npol; pol++)
            {              
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
    //printf("\n\nnbit: %d ntimesteps: %lu nchan: %lu npol: %d out_buffer_bytes: %lu\n\n", ctx->nbit/8, ctx->beams[beam].ntimesteps, ctx->beams[beam].nchan, ctx->npol, out_buffer_bytes);        
    if (create_fil_block(client, &(ctx->beams[beam].out_filfile_ptr), ctx->nbit/8, ctx->beams[beam].ntimesteps, 
                          ctx->beams[beam].nchan, ctx->npol, (float*)buffer, out_buffer_bytes))    
    {
      // Error!
      multilog(log, LOG_ERR, "dada_dbfil_io(): Error Writing into new fil block (beam %d).\n", beam + 1);
      return -1;
    }
    else
    {   
      wrote = out_buffer_bytes;
      written += wrote;
      
      // If this beam is the last beam then increment the marker number
      if (beam == ctx->nbeams_total - 1)
        ctx->obs_marker_number += 1; 

      ctx->block_number += 1;    
      ctx->bytes_written += written;

      if (ctx->stats_dir != NULL)
      {
        /* Make a new filename for the freq stats */
        char output_spectrum_filename[PATH_MAX];
        snprintf(output_spectrum_filename, PATH_MAX, "%s/%ld_ch%02d_%02d_%03d_spec.txt", 
                ctx->stats_dir, ctx->obs_id, ctx->coarse_channel, beam + 1, ctx->obs_marker_number);

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
                ctx->stats_dir, ctx->obs_id, ctx->coarse_channel, beam + 1, ctx->obs_marker_number);

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
    if (ctx->exposure_sec == ctx->obs_offset + ctx->secs_per_subobs)
    {            
      do_close_file = 1;
    }    
    else
    {
      // We hit the end of the ring buffer, but we shouldn't have. Put out a warning
      multilog(log, LOG_ERR, "dada_dbfil_close(): We hit the end of the ring buffer, but we shouldn't have! EXPOSURE_SEC=%d but this block OBS_OFFSET=%d and ends at %d sec.\n", ctx->exposure_sec,ctx->obs_offset, ctx->obs_offset + ctx->secs_per_subobs);
      exit(-1);
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
    // Close existing fil files (if we have any)    
    for (int beam=0; beam < ctx->nbeams_total; beam++)
    {
      if (&(ctx->beams[beam].out_filfile_ptr) != NULL)
      {
        multilog(log, LOG_INFO, "dada_dbfil_close(): Closing %s...\n", ctx->beams[beam].fil_filename);

        if (close_fil(client, &(ctx->beams[beam].out_filfile_ptr))) 
        {
          multilog(log, LOG_ERR,"dada_dbfil_close(): Error closing fil file.\n");
          return -1;
        }
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
  ctx->nbeams_incoherent = 0;
  ctx->nbeams_coherent = 0;
  ctx->nbeams_total = 0;
  ctx->transfer_size = 0;
  ctx->coarse_channel = 0;
    
  if (ascii_header_get(client->header, HEADER_UTC_START, "%s", &ctx->utc_start) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_UTC_START);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_OBS_OFFSET, "%i", &ctx->obs_offset) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_OBS_OFFSET);
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

  if (ascii_header_get(client->header, HEADER_BANDWIDTH_HZ, "%i", &ctx->bandwidth_hz) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_BANDWIDTH_HZ);
    return -1;
  }  

  if (ascii_header_get(client->header, HEADER_EXPOSURE_SECS, "%i", &ctx->exposure_sec) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_EXPOSURE_SECS);
    return -1;
  }  
  
  if (ascii_header_get(client->header, HEADER_NUM_INCOHERENT_BEAMS, "%i", &ctx->nbeams_incoherent) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_NUM_INCOHERENT_BEAMS);
    return -1;
  }  

  if (ascii_header_get(client->header, HEADER_NUM_COHERENT_BEAMS, "%i", &ctx->nbeams_coherent) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_NUM_COHERENT_BEAMS);
    return -1;
  }  

  if (ascii_header_get(client->header, HEADER_SECS_PER_SUBOBS, "%i", &ctx->secs_per_subobs) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_SECS_PER_SUBOBS);
    return -1;
  }

  if (ascii_header_get(client->header, HEADER_MC_IP, "%s", &ctx->multicast_ip) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_BANDWIDTH_HZ);
    return -1;
  }  

  if (ascii_header_get(client->header, HEADER_MC_PORT, "%i", &ctx->multicast_port) == -1)
  {
    multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", HEADER_BANDWIDTH_HZ);
    return -1;
  }    

  /* seconds per sub observation must be > 0 */
  if (!(ctx->secs_per_subobs > 0))
  {
    multilog(log, LOG_ERR, "dada_dbfits_open(): %s is not greater than 0.\n", HEADER_SECS_PER_SUBOBS);
    return -1;
  }

  /* ensure beams is sane */
  if (ctx->nbeams_incoherent <= 0 && ctx->nbeams_coherent <= 0)
  {
    multilog(log, LOG_ERR, "dada_dbfits_open(): There are no beams in this subobservation.\n");
    return -1;
  }

  if (ctx->nbeams_incoherent > INCOHERENT_BEAMS_MAX)
  {
    multilog(log, LOG_ERR, "dada_dbfits_open(): %s is not greater than 0 and less than or equal to %d.\n", HEADER_NUM_INCOHERENT_BEAMS, INCOHERENT_BEAMS_MAX);
    return -1;
  }

  if (ctx->nbeams_coherent > COHERENT_BEAMS_MAX)
  {
    multilog(log, LOG_ERR, "dada_dbfits_open(): %s is not greater than 0 and less than or equal to %d.\n", HEADER_NUM_COHERENT_BEAMS, COHERENT_BEAMS_MAX);
    return -1;
  }  

  // Process Beams
  ctx->nbeams_total = ctx->nbeams_incoherent + ctx->nbeams_coherent;

  // Free previous beam structure
  if (ctx->beams != 0)
    free(ctx->beams);
  
  // Allocate beams
  ctx->beams = malloc(ctx->nbeams_total * sizeof(beam_s));
  
  ctx->expected_transfer_size = 0;

  for (int beam_index = 0; beam_index < ctx->nbeams_total; beam_index++)
  {                    
    if (ascii_header_get(client->header, incoherent_beam_time_integ_string[beam_index], "%ld", &ctx->beams[beam_index].time_integration) == -1)
    {
      multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", incoherent_beam_time_integ_string[beam_index]);
      return -1;
    }
    else
    {
      ctx->beams[beam_index].beam_type = incoherent;
    }
          
    
    if (ascii_header_get(client->header, incoherent_beam_fine_chan_string[beam_index], "%ld", &ctx->beams[beam_index].nchan) == -1)
    {
      multilog(log, LOG_ERR, "read_dada_header(): %s not found in header.\n", incoherent_beam_fine_chan_string[beam_index]);
      return -1;
    }
    else
    {
      ctx->beams[beam_index].beam_type = incoherent;
    }  

    switch (ctx->beams[beam_index].beam_type)
    {
      case incoherent:
        ctx->beams[beam_index].ntimesteps = (long)ctx->bandwidth_hz / ctx->beams[beam_index].time_integration / ctx->beams[beam_index].nchan;        
        ctx->expected_transfer_size = ctx->expected_transfer_size + 
                                      (ctx->beams[beam_index].ntimesteps * ctx->beams[beam_index].nchan * 
                                      ctx->npol * (ctx->nbit/8));
        break;

      case coherent:
        multilog(log, LOG_ERR, "read_dada_header(): Coherent beam not supported (beam index %d).\n", beam_index);
        return -1;
        //break;

      case unknown:
        multilog(log, LOG_ERR, "read_dada_header(): Cannot determine beam type for beam index %d.\n", beam_index);
        return -1;
    }
  }

  // Calculate start freq of each fine channel

  //
  // MWA Coarse channel * Bandwidth = Center of channel
  //
  long start_chan_hz = (ctx->coarse_channel * ctx->bandwidth_hz);
  
  // allocate space for the fine channel frequencies for each beam
  for (int beam=0; beam < ctx->nbeams_incoherent; beam++)
  {
    if (ctx->beams[beam].channels != 0)
      free(ctx->beams[beam].channels);

    ctx->beams[beam].channels = calloc(ctx->beams[beam].nchan, sizeof(double));

    int fine_chan_width_hz = ctx->bandwidth_hz / ctx->beams[beam].nchan;
    
    for (int ch=0; ch < ctx->beams[beam].nchan; ch++)
    {    
      ctx->beams[beam].channels[ch] = (start_chan_hz + (ch * fine_chan_width_hz)) / 1000000.0;
    }
  }

  // Output what we found in the header
  multilog(log, LOG_INFO, "Obs Id:                     %lu\n", ctx->obs_id);
  multilog(log, LOG_INFO, "Subobs Id:                  %lu\n", ctx->subobs_id);
  multilog(log, LOG_INFO, "Offset:                     %d sec\n", ctx->obs_offset);
  multilog(log, LOG_INFO, "Command:                    %s\n", ctx->command);  
  multilog(log, LOG_INFO, "Start time (UTC):           %s\n", ctx->utc_start);  
  multilog(log, LOG_INFO, "Duration (secs):            %d\n", ctx->exposure_sec);    
  multilog(log, LOG_INFO, "Bits per real/imag:         %d\n", ctx->nbit);  
  multilog(log, LOG_INFO, "Polarisations:              %d\n", ctx->npol);
  multilog(log, LOG_INFO, "Coarse channel no.:         %d\n", ctx->coarse_channel);      
  multilog(log, LOG_INFO, "Coarse Channel Bandwidth:   %d Hz\n", ctx->bandwidth_hz);   
  multilog(log, LOG_INFO, "Size of subobservation:     %lu bytes\n", ctx->transfer_size); 
  multilog(log, LOG_INFO, "Expected Size of 1s block:  %lu bytes\n", ctx->expected_transfer_size); 
  
  multilog(log, LOG_INFO, "Total Beams:                %d\n", ctx->nbeams_total);  
  multilog(log, LOG_INFO, "Incoheremt Beams:           %d\n", ctx->nbeams_incoherent);  
  
  for (int beam=0; beam < ctx->nbeams_incoherent; beam++)
  {
    multilog(log, LOG_INFO, "..Beam %.2d time int (tscrunch): %ld\n", beam+1, ctx->beams[beam].time_integration);
    multilog(log, LOG_INFO, "..Beam %.2d timesteps/sec:       %ld\n", beam+1, ctx->beams[beam].ntimesteps);    
    multilog(log, LOG_INFO, "..Beam %.2d channels:            %d\n", beam+1, ctx->beams[beam].nchan);    

    for (int ch=0; ch < ctx->beams[0].nchan; ch++)
    {
      // Enable this for debug! If lots of channels it can be big!
      //multilog(log, LOG_INFO, "..Beam %.2d ch %d %f MHz\n", beam+1, ch, ctx->beams[beam].channels[ch]);
    }
  }

  multilog(log, LOG_INFO, "Coheremt Beams:             %d\n", ctx->nbeams_coherent);  
  
  for (int beam=0; beam < ctx->nbeams_coherent; beam++)
  {
    multilog(log, LOG_INFO, "..Beam %.2d time int (tscrunch): %ld\n", beam+1, ctx->beams[beam].time_integration);
    multilog(log, LOG_INFO, "..Beam %.2d timesteps/sec:       %ld\n", beam+1, ctx->beams[beam].ntimesteps);    
    multilog(log, LOG_INFO, "..Beam %.2d channels:            %d\n", beam+1, ctx->beams[beam].nchan);    

    for (int ch=0; ch < ctx->beams[0].nchan; ch++)
    {
      // Enable this for debug! If lots of channels it can be big!
      //multilog(log, LOG_INFO, "..Beam %.2d ch %d %f MHz\n", beam+1, ch, ctx->beams[beam].channels[ch]);
    }
  }

  multilog(log, LOG_INFO, "Multicast IP:               %s\n", ctx->multicast_ip);    
  multilog(log, LOG_INFO, "Multicast Port:             %d\n", ctx->multicast_port);    
    
  return EXIT_SUCCESS;
}
