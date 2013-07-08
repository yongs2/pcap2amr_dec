#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

typedef unsigned int uint32;
typedef unsigned short int uint16;
typedef signed int int32;
typedef signed short int int16;

int b_is_rtp = 0;
int b_octet_align = 1;
int dtx = 0;
int mode = 7;
int ptime = 20;
int nAmrCodec = 0;

int amrnb_decode_init();
int amrnb_decode_uninit();
int amrnb_read_bytes(int nMode);
int amrnb_decode(char *pData, int nSize, FILE *fp);

int amrwb_decode_init();
int amrwb_decode_uninit();
int amrwb_read_bytes(int nMode);
int amrwb_decode(char *pData, int nSize, FILE *fp);

int main(int argc, char *argv[])
{
	int nRet = 0, nSize = 0, nCount = 0;
	char szInputFileName[256] = "";
	char szOutputFileName[256] = "";
	FILE *fp = NULL, *fout = NULL;
	int nAmrSize = 32;
	unsigned char cData[65536];

	if(argc <= 1)
	{
		printf("Usage: %s [amr file] [nb|wb] [octet-aligned] \n", argv[0]);
		exit(0);
	}
	if(argc > 1)
	{
		strcpy(szInputFileName, argv[1]);
		strcpy(szOutputFileName, basename(szInputFileName));
		strcat(szOutputFileName, ".pcm");
	}
	if(argc > 2)
	{
		if(strcasecmp(argv[2], "nb") == 0)
		{
			nAmrCodec = 0;	// amr-mb
			mode = 7;
			nAmrSize = 31+1;
		}
		else if(strcasecmp(argv[2], "wb") == 0)
		{
			nAmrCodec = 1;	// amr-wb
			mode = 8;
			nAmrSize = 60+1;
		}
	}
	if(argc > 3)
	{
		mode = atoi(argv[3]);
		if(nAmrCodec == 0)
		{
			nAmrSize = amrnb_read_bytes(mode) + 1;
		}
		else
		{
			nAmrSize = amrwb_read_bytes(mode) + 1;
		}
	}

	printf("AMR File=[%s], Output=[%s], amr-%s, mode=%d, octet-aligned=%d\n", 
		szInputFileName, szOutputFileName, 
		(nAmrCodec == 0) ? "nb" : "wb",
		mode, b_octet_align);

	fp = fopen(szInputFileName, "rb");
	if(fp == NULL)
		return -1;

	fout = fopen(szOutputFileName, "wb");
	if(fout == NULL)
		return -1;

	if(nAmrCodec == 0)
	{
		const char szFileHeader[] = "#!AMR\n";
		amrnb_decode_init();
		nRet = fread(cData, (size_t)1, strlen(szFileHeader), fp);
	}
	else
	{
		const char szFileHeader[] = "#!AMR-WB\n";
		amrwb_decode_init();
		nRet = fread(cData, (size_t)1, strlen(szFileHeader), fp);
	}

	while(1)
	{
		// Read AMR Data
		memset(&cData, 0, sizeof(cData));
		nRet = fread(cData, (size_t)1, (size_t)nAmrSize, fp);
		printf("AMR.data(%d).read=%d\n", nAmrSize, nRet);
		if(nRet > 0)
		{
			nSize = nRet;
			printf("AMR.amr_decode(0x%x, %d)............\n", (unsigned char)(cData), nSize);
			if(nAmrCodec == 0)
			{
				nRet = amrnb_decode((char *)(cData), nSize, fout);
			}
			else
			{
				nRet = amrwb_decode((char *)(cData), nSize, fout);
			}
			printf("AMR.amr_decode(0x%x, %d)=%d\n", (unsigned char)cData, nSize, nRet);
		}
		else
		{
			break;
		}
		nCount++;
	}
	fclose(fout);
	fclose(fp);

	if(nAmrCodec == 0)
	{
		amrnb_decode_uninit();
	}
	else
	{
		amrwb_decode_uninit();
	}

	return 0;
}
