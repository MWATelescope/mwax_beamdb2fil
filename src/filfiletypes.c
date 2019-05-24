// PROGAM WRITTERN BY MARCIN SOKOLOWSKI (May 2019) , marcin.sokolowski@curtin.edu.au
//  It converts raw files in psrdada format into .fil (filterbank) file format 
//  filfiletypes.h(cpp) - define .fil (filterbank) data types at the moment not really used
#include "filfiletypes.h"

// grep strcmp ../read_filfile.c  | awk '{ ind=index($0,"\"");line=substr($0,ind+1);end=index(line,"\"");key=substr(line,0,end-1);type=substr($7,2,1);type_enum="eFilHdrUnknown";if(type=="f"){type_enum="eFilHdrFlag";}if(type=="i"){type_enum="eFilHdrInt";} if(type=="s"){type_enum="eFilHdrStr";} if(type=="d"){type_enum="eFilHdrDouble";}  if(type=="b"){type_enum="eFilHdrBool";}  print "{ \""key"\" , "type_enum " },";}'
// !!! IMPORTANT NOTE : order has to be the same as in : enum eFilHeaderKey in filfiletypes.h file 
eFilKeyword gFilFileKeywordTypes[]  = 
{
   { "HEADER_START" , eFilHdrFlag },
   { "telescope_id" , eFilHdrInt },
   { "machine_id" , eFilHdrInt },
   { "data_type" , eFilHdrInt },
   { "rawdatafile" , eFilHdrStr },
   { "source_name" , eFilHdrStr },
   { "barycentric" , eFilHdrInt },
   { "pulsarcentric" , eFilHdrInt },
   { "az_start" , eFilHdrDouble },
   { "za_start" , eFilHdrDouble },
   { "src_raj" , eFilHdrDouble },
   { "src_dej" , eFilHdrDouble },
   { "tstart" , eFilHdrDouble },
   { "tsamp" , eFilHdrDouble },
   { "nbits" , eFilHdrInt },
   { "signed" , eFilHdrBool },
   { "nsamples" , eFilHdrInt },
   { "nbeams" , eFilHdrInt },
   { "ibeam" , eFilHdrInt },
   { "fch1" , eFilHdrDouble },
   { "foff" , eFilHdrDouble },
   { "FREQUENCY_START" , eFilHdrFlag },
   { "fchannel" , eFilHdrDouble },
   { "FREQUENCY_END" , eFilHdrFlag },
   { "nchans" , eFilHdrInt },
   { "nifs" , eFilHdrInt },
   { "refdm" , eFilHdrDouble },
   { "period" , eFilHdrDouble },
   { "npuls" , eFilHdrUnknown },
   { "nbins" , eFilHdrInt },
   { "HEADER_END" , eFilHdrFlag }
};
