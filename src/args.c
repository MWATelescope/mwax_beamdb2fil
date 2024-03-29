/**
 * @file args.c
 * @author Greg Sleap
 * @date 21 May 2018
 * @brief This is the code that parses and validates command line arguments
 *
 */
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include "args.h"
#include "global.h"
#include "version.h"

/**
 * 
 *  @brief This function parses command line arguments. Returns success if all good.
 *  @param[in] argc Count of arguments passed from main()
 *  @param[in] argv[] Array of arguments passed from main()
 *  @param[in] globalArgs Pointer to the structure where we put the parsed arguments.
 *  @returns EXIT_SUCCESS on success, or -1 if there was an error.
 */
int process_args(int argc, char *argv[], globalArgs_s *globalArgs)
{
    globalArgs->input_db_key = 0;
    globalArgs->metafits_path = NULL;
    globalArgs->destination_path = NULL;
    globalArgs->health_ip = NULL;
    globalArgs->health_port = 0;
    globalArgs->stats_path = NULL;

    static const char *optString = "k:m:d:i:p:?";

    static const struct option longOpts[] =
        {
            {"key", required_argument, NULL, 'k'},
            {"metafits-path", required_argument, NULL, 'm'},
            {"destination-path", required_argument, NULL, 'd'},
            {"health-ip", required_argument, NULL, 'i'},
            {"health-port", required_argument, NULL, 'p'},
            {"stats-path", optional_argument, NULL, 's'},
            {"help", no_argument, NULL, '?'},
            {NULL, no_argument, NULL, 0}};

    int opt = 0;
    int longIndex = 0;

    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    while (opt != -1)
    {
        switch (opt)
        {
        case 'k':
            globalArgs->input_db_key = strtol(optarg, NULL, 16);
            break;

        case 'd':
            globalArgs->destination_path = optarg;
            break;

        case 'm':
            globalArgs->metafits_path = optarg;
            break;

        case 'i':
            globalArgs->health_ip = optarg;
            break;

        case 'p':
            globalArgs->health_port = atoi(optarg);
            break;

        case 's':
            globalArgs->stats_path = optarg;
            break;

        case '?':
            print_usage();
            return EXIT_FAILURE;

        default:
            /* You won't actually get here. */
            break;
        }

        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }

    // Check that mandatory parameters are passed
    if (!globalArgs->input_db_key)
    {
        fprintf(stderr, "Error: input shared memory key (-k | --key) is mandatory.\n");
        print_usage();
        exit(1);
    }

    if (!globalArgs->metafits_path)
    {
        fprintf(stderr, "Error: metafits path (-m | --metafits-path) is mandatory.\n");
        print_usage();
        exit(1);
    }

    if (!globalArgs->destination_path)
    {
        fprintf(stderr, "Error: destination path (-d | --destination-path) is mandatory.\n");
        print_usage();
        exit(1);
    }

    if (!globalArgs->health_ip)
    {
        fprintf(stderr, "Error: health ip (-i | --health-ip) is mandatory.\n");
        print_usage();
        exit(1);
    }

    if (!globalArgs->health_port)
    {
        fprintf(stderr, "Error: health port (-p | --health-port) is mandatory.\n");
        print_usage();
        exit(1);
    }

    return EXIT_SUCCESS;
}

/**
 * 
 *  @brief This is called to provide the user with the summary of usage/help
 */
void print_usage()
{
    print_version();
    printf("\nUsage: mwax_beamdb2fil [OPTION]...\n\n");
    printf("This code will open the dada ringbuffer containing beam \n");
    printf("data from the MWAX beamformer.\n");
    printf("It will then write out a filterbank (fil) file to the destination dir.\n\n");
    printf("  -k --key=KEY                Hexadecimal shared memory key\n");
    printf("  -d --destination-path=PATH  Destination path for gpubox files\n");
    printf("  -m --metafits-path=PATH     Metafits directory path\n");
    printf("  -i --health-ip=IP           Health UDP destination ip address\n");
    printf("  -p --health-port=PORT       Health UDP destination port\n");
    printf("  -s --stats-path=PATH        (Optional) Statistics directory path\n");
    printf("  -? --help                   This help text\n");
}

/**
 * 
 *  @brief This is called to provide the user with the version info
 */
void print_version()
{
    printf("mwax_beamdb2fil v%d.%d.%d\n", MWAX_BEAMDB2FIL_VERSION_MAJOR, MWAX_BEAMDB2FIL_VERSION_MINOR, MWAX_BEAMDB2FIL_VERSION_PATCH);
}