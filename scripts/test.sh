#
# test
#
# Run this to test mwax_beamdb2fil
#

# 1 kHz
# 1 ms
# in 1 sec blocks
# and the duration is 8 seconds
# So this means:
# 8 blocks
# each block should contain:
# 1000 x 1ms timesteps
# Each timestep contains 1280/1 channels == 1280 channels
# Each channel has a float
# So!
# 1 value for 1 channel for 1 timestep  == 4 bytes
# 1 timestep (containing 1280 channels) == 4 * 1,280                   == 5,120 bytes
# 1 second (containing 1000 timesteps)  == 4 * 1,280 * 1,000 bytes     == 5,120,000 bytes
# 8 seconds                             == 4 * 1,280 * 1,000 * 8 bytes == 40,960,000 bytes
# 
# However, Ian's file is dumped from db_disk where the ringbuffer was 4x this, so: 163,840,000
# or 20,480,000 per second, which is the block size we need in dada_db command.
#
# clean up
echo Cleaning up ringbuffer...
dada_db -d -k 5678

echo Instantiating input ring buffer...
dada_db -b 20480000 -k 5678 -n 16 

echo Loading test dada $1 file into ringbuffer...
dada_diskdb -k 5678 -f $1

# executing beamdb2fil
echo Starting beamdb2fil...
../bin/mwax_beamdb2fil -k 5678 --destination-path=. --health-ip=127.0.0.1 --health-port=7123

# clean up
echo Cleaning up ringbuffer
dada_db -d -k 5678
