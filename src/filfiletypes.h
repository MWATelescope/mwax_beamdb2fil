#pragma once

typedef enum eFilHeaderType  
{
   eFilHdrUnknown = 0,
   eFilHdrFlag    = 1,
   eFilHdrInt     = 2,
   eFilHdrStr     = 3,
   eFilHdrDouble  = 4,
   eFilHdrBool    = 5
} eFilHeaderType;

// grep strcmp ../read_filfile.c  | awk '{ ind=index($0,"\"");line=substr($0,ind+1);end=index(line,"\"");key=substr(line,0,end-1);type=substr($7,2,1);type_enum="eFilHdrUnknown";if(type=="f"){type_enum="eFilHdrFlag";}if(type=="i"){type_enum="eFilHdrInt";} if(type=="s"){type_enum="eFilHdrStr";} if(type=="d"){type_enum="eFilHdrDouble";}  if(type=="b"){type_enum="eFilHdrBool";}  print "enum_"key" = " NR",";}'
typedef enum eFilHeaderKey 
{
   enum_HEADER_START    = 1,
   enum_telescope_id    = 2,
   enum_machine_id      = 3,
   enum_data_type       = 4,
   enum_rawdatafile     = 5,
   enum_source_name     = 6,
   enum_barycentric     = 7,
   enum_pulsarcentric   = 8,
   enum_az_start        = 9,
   enum_za_start        = 10,
   enum_src_raj         = 11,
   enum_src_dej         = 12,
   enum_tstart          = 13,
   enum_tsamp           = 14,
   enum_nbits           = 15,
   enum_nsamples        = 16,
   enum_fch1            = 17,
   enum_foff            = 18,
   enum_nchans          = 19,
   enum_nifs            = 20,
   enum_refdm           = 21,
   enum_period          = 22,
   enum_nbeams          = 23,
   enum_ibeam           = 24,
   enum_HEADER_END      = 25
} eFilHeaderKey;

// grep strcmp ../read_filfile.c  | awk '{ ind=index($0,"\"");line=substr($0,ind+1);end=index(line,"\"");key=substr(line,0,end-1);type=substr($7,2,1);type_enum="eFilHdrUnknown";if(type=="f"){type_enum="eFilHdrFlag";}if(type=="i"){type_enum="eFilHdrInt";} if(type=="s"){type_enum="eFilHdrStr";} if(type=="d"){type_enum="eFilHdrDouble";}  if(type=="b"){type_enum="eFilHdrBool";}  print "{ \""key"\" , "type_enum " },";}'
typedef struct eFilKeyword
{
   const char* keyword;
   enum eFilHeaderType type;
} eFilKeyword;
