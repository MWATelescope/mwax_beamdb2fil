#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "filfile.h"

//
// CFilFile
//
int CFilFile_Open(cFilFile *filfile_ptr, char* filename)
{
    if( !filfile_ptr->m_File )
    {                
        filfile_ptr->m_szFileName = filename;
        
        filfile_ptr->m_File = fopen(filfile_ptr->m_szFileName, "wb" );
    }
   
    return EXIT_SUCCESS;
}

int CFilFile_Close(cFilFile *filfile_ptr)
{
    if (filfile_ptr->m_File)
    {
        fclose(filfile_ptr->m_File);
    }

    return EXIT_SUCCESS;
}

void CFilFile_CheckFile(cFilFile *filfile_ptr)
{
    if( !filfile_ptr->m_File )
    {
        printf("ERROR : file has not been open (requested name = %s) -> exiting !\n", filfile_ptr->m_szFileName);
        exit(-1);
    }
}

// HEADER :
int CFilFile_WriteHeader(cFilFile *filfile_ptr, const cFilFileHeader* filHeader )
{      
    CFilFile_WriteKeyword_string(filfile_ptr, "HEADER_START", NULL);

    // awk '{print "   WriteKeyword( \""$2"\" , filHeader."$2" );";}' keys.tmp > keys.txt
    CFilFile_WriteKeyword_int(filfile_ptr, "telescope_id" , filHeader->telescope_id );
    CFilFile_WriteKeyword_int(filfile_ptr, "machine_id" , filHeader->machine_id );
    CFilFile_WriteKeyword_int(filfile_ptr, "data_type" , filHeader->data_type );
    CFilFile_WriteKeyword_string(filfile_ptr, "rawdatafile" , filHeader->rawdatafile );
    CFilFile_WriteKeyword_string(filfile_ptr, "source_name" , filHeader->source_name );
    CFilFile_WriteKeyword_int(filfile_ptr, "barycentric" , filHeader->barycentric );
    CFilFile_WriteKeyword_int(filfile_ptr, "pulsarcentric" , filHeader->pulsarcentric );
    CFilFile_WriteKeyword_double(filfile_ptr, "az_start" , filHeader->az_start );
    CFilFile_WriteKeyword_double(filfile_ptr, "za_start" , filHeader->za_start );
    CFilFile_WriteKeyword_double(filfile_ptr, "src_raj" , filHeader->src_raj );
    CFilFile_WriteKeyword_double(filfile_ptr, "src_dej" , filHeader->src_dej );
    CFilFile_WriteKeyword_double(filfile_ptr, "tstart" , filHeader->tstart );
    CFilFile_WriteKeyword_double(filfile_ptr, "tsamp" , filHeader->tsamp );
    CFilFile_WriteKeyword_int(filfile_ptr, "nbits" , filHeader->nbits );
    CFilFile_WriteKeyword_string(filfile_ptr, "signed" , filHeader->signed_ );
    CFilFile_WriteKeyword_int(filfile_ptr, "nsamples" , filHeader->nsamples );
    CFilFile_WriteKeyword_int(filfile_ptr, "nbeams" , filHeader->nbeams );
    CFilFile_WriteKeyword_int(filfile_ptr, "ibeam" , filHeader->ibeam );
    CFilFile_WriteKeyword_double(filfile_ptr, "fch1" , filHeader->fch1 );
    CFilFile_WriteKeyword_double(filfile_ptr, "foff" , filHeader->foff );
    
    CFilFile_WriteKeyword_string(filfile_ptr, "FREQUENCY_START", NULL);

    for (int ch=0; ch<filHeader->nchans; ch++)
    {
        CFilFile_WriteKeyword_double(filfile_ptr, "fchannel" , filHeader->fchannel[ch] );
    }

    CFilFile_WriteKeyword_string(filfile_ptr, "FREQUENCY_END", NULL);

    CFilFile_WriteKeyword_int(filfile_ptr, "nchans" , filHeader->nchans );
    CFilFile_WriteKeyword_int(filfile_ptr, "nifs" , filHeader->nifs );
    CFilFile_WriteKeyword_double(filfile_ptr, "refdm" , filHeader->refdm );
    CFilFile_WriteKeyword_double(filfile_ptr, "period" , filHeader->period );
    CFilFile_WriteKeyword_longlong(filfile_ptr, "npuls" , filHeader->npuls );
    CFilFile_WriteKeyword_int(filfile_ptr, "nbins" , filHeader->nbins );
        
    CFilFile_WriteKeyword_string(filfile_ptr, "HEADER_END", NULL);

    return EXIT_SUCCESS;
}

int CFilFile_WriteString(cFilFile *filfile_ptr, const char* keyname )
{
    CFilFile_CheckFile(filfile_ptr);

    // write length of keyword name 
    int len = strlen( keyname );
    size_t ret = fwrite( &len, 1, sizeof(len), filfile_ptr->m_File );

    ret += fwrite( keyname, 1, len, filfile_ptr->m_File );

    if( ret != (strlen(keyname)+sizeof(int)) ){
        printf("ERROR : error in code did not write keyword name %s correctly (written %lu bytes, expected %lu bytes)\n",
               keyname, ret, strlen(keyname)+sizeof(int));
        exit(-1);
    }

    return ret;
}

int CFilFile_WriteKeyword_int(cFilFile *filfile_ptr, const char* keyname, int iValue)
{
    int ret = CFilFile_WriteString(filfile_ptr, keyname );

    ret += fwrite( &iValue, 1, sizeof(iValue), filfile_ptr->m_File );

    return ret;
}

int CFilFile_WriteKeyword_double(cFilFile *filfile_ptr, const char* keyname, double dValue)
{
    int ret = CFilFile_WriteString(filfile_ptr, keyname );
    ret += fwrite( &dValue, 1, sizeof(dValue), filfile_ptr->m_File );

    return ret;
}

int CFilFile_WriteKeyword_longlong(cFilFile *filfile_ptr, const char* keyname, long long llValue)
{
    int ret = CFilFile_WriteString(filfile_ptr, keyname );
    ret += fwrite( &llValue, 1, sizeof(llValue), filfile_ptr->m_File );

    return ret;
}

int CFilFile_WriteKeyword_string(cFilFile *filfile_ptr, const char* keyname, const char* szValue)
{
    int ret = CFilFile_WriteString(filfile_ptr, keyname );

    if( szValue && strlen(szValue) ){
        ret += CFilFile_WriteString(filfile_ptr, szValue );       
    }

    return ret;
}

// DATA :
int CFilFile_WriteData(cFilFile *filfile_ptr, float* data_float, int n_floats )
{
    int ret = fwrite( data_float, sizeof(float), n_floats, filfile_ptr->m_File );

    return ret;
}

//
// CFilFileHeader
//
void CFilFileHeader_Constructor(cFilFileHeader* filefileheader_ptr)   
{
    filefileheader_ptr->telescope_id = 0;    
    filefileheader_ptr->machine_id = 0;
    filefileheader_ptr->data_type = 0;
    strncpy(filefileheader_ptr->rawdatafile, "", 0);
    strncpy(filefileheader_ptr->source_name, "", 0);
    filefileheader_ptr->barycentric = 0;
    filefileheader_ptr->pulsarcentric = 0;
    filefileheader_ptr->az_start = 0.00;
    filefileheader_ptr->za_start = 0.00;
    filefileheader_ptr->src_raj = 0.00;
    filefileheader_ptr->src_dej = 0.00;
    filefileheader_ptr->tstart = 0.00;
    filefileheader_ptr->tsamp = 0.00;
    filefileheader_ptr->nbits = 4;
    filefileheader_ptr->signed_[0] = 1;        // signed is a restricted keyword in C/C++ -> added _
    filefileheader_ptr->nsamples = 0;
    filefileheader_ptr->nbeams = 1;
    filefileheader_ptr->ibeam = 0;
    filefileheader_ptr->fch1 = 0,
    filefileheader_ptr->foff = -0.001;
    filefileheader_ptr->fchannel = NULL;
    filefileheader_ptr->nchans = 0;
    filefileheader_ptr->nifs = 0;
    filefileheader_ptr->refdm = 0;
    filefileheader_ptr->period = 0;
    filefileheader_ptr->npuls = 0;
    filefileheader_ptr->nbins = 0;
}

