/**
 * @file dada_dbfil.h
 * @author Greg Sleap
 * @date 23 May 2018
 * @brief This is the header for the code that drives the ring buffers
 *
 */
#pragma once

#include "dada_client.h"

// function prototypes
int dada_dbfil_open(dada_client_t *client);
int dada_dbfil_close(dada_client_t *client, uint64_t bytes_written);
int64_t dada_dbfil_io(dada_client_t *client, void *buffer, uint64_t bytes);
int64_t dada_dbfil_io_block(dada_client_t *client, void *buffer, uint64_t bytes, uint64_t block_id);
int read_dada_header(dada_client_t *client);