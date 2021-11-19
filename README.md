# mwax-beamdb2fil
MWA (mwax) Beam PSRDADA ringbuffer datablock to filterbank file

## Dependencies
### CFITSIO 
- See https://heasarc.gsfc.nasa.gov/fitsio/fitsio.html
### psrdada prerequisites:
- pkg-config
- libhwloc-dev (this is to enable the use of NUMA awareness in psrdada)
- csh
- autoconf
- libtool
### psrdada (http://psrdada.sourceforge.net/)
- download the source from the cvs repo (http://psrdada.sourceforge.net/current/)
- build (http://psrdada.sourceforge.net/current/build.shtml)

## Building
```
$ cmake CMakeLists.txt
$ make
```

## Running / Command line Arguments
Example from `mwax_beamdb2fil --help`
```
mwax_beamdb2fil v0.10.0

Usage: mwax_beamdb2fil [OPTION]...

This code will open the dada ringbuffer containing beam
data from the MWAX beamformer.
It will then write out a filterbank (fil) file to the destination dir.

  -k --key=KEY                Hexadecimal shared memory key
  -d --destination-path=PATH  Destination path for gpubox files
  -m --metafits-path=PATH     Metafits directory path
  -i --health-ip=IP           Health UDP destination ip address
  -p --health-port=PORT       Health UDP destination port
  -s --stats-path=PATH        (Optional) Statistics directory path
  -? --help                   This help text
```