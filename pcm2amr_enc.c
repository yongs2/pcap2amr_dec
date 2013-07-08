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

int amrnb_encode_init();
int amrnb_encode_uninit();
int amrnb_encode(char *pData, int nSize, FILE *fp);

int amrwb_encode_init();
int amrwb_encode_uninit();
int amrwb_encode(char *pData, int nSize, FILE *fp);

int main(int argc, char *argv[])
{
	int nRet = 0, nSize = 0, nCount = 0;
	char szInputFileName[256] = "";
	char szOutputFileName[256] = "";
	FILE *fp = NULL, *fout = NULL;
	int nPcmSize = 320;
	unsigned char cData[65536];

	if(argc <= 1)
	{
		printf("Usage: %s [pcm file] [nb|wb] [octet-aligned] \n", argv[0]);
		exit(0);
	}
	if(argc > 1)
	{
		strcpy(szInputFileName, argv[1]);
		strcpy(szOutputFileName, basename(szInputFileName));
		strcat(szOutputFileName, ".amr");
	}
	if(argc > 2)
	{
		if(strcasecmp(argv[2], "nb") == 0)
		{
			nAmrCodec = 0;	// amr-mb
			mode = 7;
			nPcmSize = 320;
		}
		else if(strcasecmp(argv[2], "wb") == 0)
		{
			nAmrCodec = 1;	// amr-wb
			mode = 8;
			nPcmSize = 640;
		}
	}
	if(argc > 3)
	{
		mode = atoi(argv[3]);
	}

	printf("PCM File=[%s], Output=[%s], amr-%s, mode=%d, octet-aligned=%d\n", 
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
		amrnb_encode_init();
		nRet = fwrite(szFileHeader, (size_t)1, strlen(szFileHeader), fout);
	}
	else
	{
		const char szFileHeader[] = "#!AMR-WB\n";
		amrwb_encode_init();
		nRet = fwrite(szFileHeader, (size_t)1, strlen(szFileHeader), fout);
	}

	while(1)
	{
		// Read PCM Data
		memset(&cData, 0, sizeof(cData));
		nRet = fread(cData, (size_t)1, (size_t)nPcmSize, fp);
		printf("PCM.data(%d).read=%d\n", nPcmSize, nRet);
		if(nRet > 0)
		{
			nSize = nRet;
			printf("PCM.amr_encode(0x%x, %d)............\n", (unsigned char)(cData), nSize);
			if(nAmrCodec == 0)
			{
				nRet = amrnb_encode((char *)(cData), nSize, fout);
			}
			else
			{
				nRet = amrwb_encode((char *)(cData), nSize, fout);
			}
			printf("PCM.amr_encode(0x%x, %d)=%d\n", (unsigned char)cData, nSize, nRet);
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
		amrnb_encode_uninit();
	}
	else
	{
		amrwb_encode_uninit();
	}

	return 0;
}
