/**
 * @file metafitsreader.h
 * @author Greg Sleap
 * @date 14 Jun 2018
 * @brief This is the header for the code that handles reading fits files
 *
 */
#pragma once

#include "fitsio.h"
#include "global.h"
#include "dada_client.h"

int open_fits(dada_client_t *client, fitsfile **fptr, const char* filename);
int read_metafits(dada_client_t *client, fitsfile *fptr_metafits, metafits_s *mptr);
int close_fits(dada_client_t *client, fitsfile **fptr);