#pragma once

#include <linux/limits.h>

// grep strcmp ../read_filfile.c  | awk '{ ind=index($0,"\"");line=substr($0,ind+1);end=index(line,"\"");key=substr(line,0,end-1);type=substr($7,2,1);type_enum="eFilHdrUnknown";if(type=="f"){type_enum="eFilHdrFlag";}if(type=="i"){type_enum="eFilHdrInt";} if(type=="s"){type_enum="eFilHdrStr";} if(type=="d"){type_enum="eFilHdrDouble";}  if(type=="b"){type_enum="eFilHdrBool";}  print "   "type_enum" "key";";}'
// see also : https://github.com/scottransom/presto/blob/master/lib/python/sigproc.py for some variables 
//      and   https://docs.python.org/3/library/struct.html for types q = C long long etc ...
typedef struct CFilFileHeader 
{
   int telescope_id;
   int machine_id;
   int data_type;
   char rawdatafile[PATH_MAX];
   char source_name[PATH_MAX];
   int barycentric;
   int pulsarcentric;
   double az_start;
   double za_start;
   double src_raj;
   double src_dej;
   double tstart;
   double tsamp;
   int nbits;
   long nsamples;
   double fch1;
   double foff;
   long nchans;
   int nifs;
   double refdm;
   double period;
   int nbeams;
   int ibeam;
} cFilFileHeader;

typedef struct CFilFile 
{
   char* m_szFileName;
   FILE*       m_File;
} cFilFile;

int CFilFile_Open(cFilFile *filfile_ptr, char* filename);   
int CFilFile_Close(cFilFile *filfile_ptr);
void CFilFile_CheckFile(cFilFile *filfile_ptr);

// HEADER :
int CFilFile_WriteHeader(cFilFile *filfile_ptr, const cFilFileHeader* filHeader );
int CFilFile_WriteString(cFilFile *filfile_ptr, const char* keyname );
int CFilFile_WriteKeyword_int(cFilFile *filfile_ptr, const char* keyname, int iValue);
int CFilFile_WriteKeyword_double(cFilFile *filfile_ptr, const char* keyname, double dValue);
int CFilFile_WriteKeyword_longlong(cFilFile *filfile_ptr, const char* keyname, long long llValue);
int CFilFile_WriteKeyword_string(cFilFile *filfile_ptr, const char* keyname, const char* szValue);

// DATA :
int CFilFile_WriteData(cFilFile *filfile_ptr, float* data_float, int n_channels );


//
// CFilFileHeader
//
void CFilFileHeader_Constructor(cFilFileHeader *filefileheader_ptr);    