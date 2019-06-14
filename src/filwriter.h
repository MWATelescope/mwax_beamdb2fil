/**
 * @file filwriter.h
 * @author Greg Sleap
 * @date 15 May 2018
 * @brief This is the header for the code that handles writing fil files
 *
 */
#pragma once

#include "dada_client.h"
#include "global.h"

int create_fil(dada_client_t *client, cFilFile *out_filfile_ptr, metafits_s *metafits);
int close_fil(dada_client_t *client, cFilFile *out_filfile_ptr);
int create_fil_block(dada_client_t *client, cFilFile *out_filfile_ptr, int bytes_per_sample, long timesteps, long fine_channels, int polarisations, float *buffer, uint64_t bytes);