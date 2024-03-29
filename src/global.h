/**
 * @file global.h
 * @author Greg Sleap
 * @date 5 Jul 2018
 * @brief This is the header for anything global 
 *
 */
#pragma once

#define HAVE_HWLOC // This is used by the psrdada library to set if we have the hwloc library or not. This lib is used to abstract NUMA / architecture.

#include <stdint.h>
#include <fitsio.h>
#include "filfile.h"
#include "multilog.h"

#define MWAX_MODE_LEN 32    // Size of the MODE in PSRDADA header. E.g. "HW_LFILES", "VOLTAGE_START", "QUIT","NO_CAPTURE"
#define UTC_START_LEN 20    // Size of UTC_START in the PSRDADA header (e.g. 2018-08-08-08:00:00)
#define HOST_NAME_LEN 64    // Length of hostname
#define IP_AS_STRING_LEN 15 // xxx.xxx.xxx.xxx

typedef enum beam_type_enum
{
    unknown = 0,
    incoherent = 1,
    coherent = 2
} beam_type_enum;

// Structure of a beam
typedef struct beam_s
{
    // FIL info
    char fil_filename[PATH_MAX];
    cFilFile out_filfile_ptr;

    // Beam settings
    long time_integration;    // i.e. time-scrunch factor, e.g. 10 means sum 10 powers samples per output
    long ntimesteps;          // how many timesteps per second
    long nchan;               // number of channels
    double *channels;         // array of fine channel centres (MHz)
    beam_type_enum beam_type; // incoherent or coherent

    // Beam Statistics
    double *power_freq; // Stats by freq
    double *power_time; // Stats by time
} beam_s;

typedef struct metafits_s
{
    long obsid;
    double azimuth;
    double altitude;
    double ra;
    double dec;
    double mjd;
    char *filename; // This is really the obs/source name
    char *channels_string;
} metafits_s;

typedef struct dada_db_s
{
    // PSRDADA stuff
    uint64_t header_size; // size of the DADA header blocks
    uint64_t block_size;  // size of the DADA data blocks
    int block_number;
    multilog_t *log;
    char *curr_block;
    char block_open;        // flag for currently open output HDU
    uint64_t bytes_written; // number of bytes currently written to output HDU
    uint64_t bytes_read;

    // Common
    char hostname[HOST_NAME_LEN + 1];
    char *destination_dir;

    // Stats
    char *stats_dir;

    // metafits
    char *metafits_path;
    char metafits_filename[PATH_MAX];
    fitsfile *in_metafits_ptr;
    metafits_s *metafits_info;

    // Observation info
    long obs_id;
    long subobs_id;
    char mode[MWAX_MODE_LEN];
    char utc_start[UTC_START_LEN];
    int obs_offset;
    int coarse_channel;
    int nbit;
    int npol;
    int bandwidth_hz;
    int nbeams_incoherent;
    int nbeams_coherent;
    int nbeams_total;
    int exposure_sec;
    uint64_t transfer_size;
    int secs_per_subobs;

    char multicast_ip[IP_AS_STRING_LEN + 1];
    int multicast_port;

    beam_s *beams;

    // Not from header- calculated values
    int obs_marker_number;
    uint64_t expected_transfer_size;
    int duration_changed;
} dada_db_s;

// Methods for the Quit mutex
int initialise_quit();
int set_quit(int value);
int get_quit();
int destroy_quit();