/**
 * @file filwriter.h
 * @author Greg Sleap
 * @date 15 May 2018
 * @brief This is the header for the code that handles writing fil files
 *
 */
#pragma once

#include "dada_client.h"
#include "filfile.h"

#define FIL_SIZE_CUTOFF_BYTES 200000

// Keys and some hard coded values for the header of the fil file produced
//#define MWA_FITS_KEY_SIMPLE "SIMPLE"

int create_fil(dada_client_t *client, cFilFile *out_filfile_ptr, const char* filename);
int close_fil(dada_client_t *client, cFilFile *out_filfile_ptr);
int create_fil_block(dada_client_t *client, cFilFile *out_filfile_ptr, int fine_channels, float *buffer, uint64_t bytes);
