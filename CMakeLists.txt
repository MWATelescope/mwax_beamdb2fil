#CMakeLists.txt
cmake_minimum_required(VERSION 3.5)
project(mwax_beamdb2fil)

add_definitions(-D_GNU_SOURCE=1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include ../mwax_common /opt/psrdada/src /opt/cfitsio/cfitsio-3.47)         # mwax  -I flags for compiler
#include_directories(${CMAKE_SOURCE_DIR}/include ../mwax_common /opt/psrdada/linux_64/include /opt/cfitsio/include) # blc0 -I flags for compiler

link_directories(${CMAKE_SOURCE_DIR}/lib /usr/local/cuda/lib64)                                                                     # mwax  -L flags for linker
#link_directories(${CMAKE_SOURCE_DIR}/lib /home/mwa/linux_64/lib /usr/local/cuda/lib64 /home/mwa/cfitsio /opt/psrdada/linux_64/lib/) # blc0 -L flags for linker

set(PROGSRC src/main.c src/args.c ../mwax_common/mwax_global_defs.c src/dada_dbfil.c src/filfile.c src/filfiletypes.c src/filwriter.c src/global.c src/health.c src/metafitsreader.c src/util.c )  # define sources

IF(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")    
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra")    
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

add_executable(mwax_beamdb2fil ${PROGSRC})       # define executable target prog, specify sources
target_link_libraries(mwax_beamdb2fil pthread cfitsio psrdada cudart m)   # -l flags for linking target
