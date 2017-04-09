#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

typedef unsigned int uint32;
typedef unsigned short int uint16;
typedef signed int int32;
typedef signed short int int16;

typedef struct pcap_hdr_s {
	uint32 magic_number;   /* magic number */
	uint16 version_major;  /* major version number */
	uint16 version_minor;  /* minor version number */
	int32  thiszone;       /* GMT to local correction */
	uint32 sigfigs;        /* accuracy of timestamps */
	uint32 snaplen;        /* max length of captured packets, in octets */
	uint32 network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
	uint32 ts_sec;         /* timestamp seconds */
	uint32 ts_usec;        /* timestamp microseconds */
	uint32 incl_len;       /* number of octets of packet saved in file */
	uint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

int b_is_rtp = 1;
int b_octet_align = 1;
int dtx = 0;
int mode = 7;
int ptime = 20;
int nAmrCodec = 0;

int amrnb_decode_init();
int amrnb_decode_uninit();
int amrnb_decode(char *pData, int nSize, FILE *fp);

int amrwb_decode_init();
int amrwb_decode_uninit();
int amrwb_decode(char *pData, int nSize, FILE *fp);

int main(int argc, char *argv[])
{
	int nRet = 0, nSize = 0, nCount = 0;
	char szInputFileName[256] = "";
	char szOutputFileName[256] = "";
	FILE *fp = NULL, *fout = NULL;
	pcap_hdr_t	stPcapHdr;
	pcaprec_hdr_t stRecHdr;
	unsigned char cData[65536];
	int nSkipHdr = 54;

	if(argc <= 1)
	{
		printf("Usage: %s [pcap file] [nb|wb] [octet-aligned] \n", argv[0]);
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
		}
		else if(strcasecmp(argv[2], "wb") == 0)
		{
			nAmrCodec = 1;	// amr-wb
			mode = 8;
		}
	}
	if(argc > 3)
	{
		b_octet_align = atoi(argv[3]);
	}

	printf("PcapFile=[%s], Output=[%s], amr-%s, octet-aligned=%d\n", 
		szInputFileName, szOutputFileName, 
		(nAmrCodec == 0) ? "nb" : "wb",
		b_octet_align);

	fp = fopen(szInputFileName, "rb");
	if(fp == NULL)
		return -1;

	fout = fopen(szOutputFileName, "wb");
	if(fout == NULL)
		return -1;

	// Read PCAP Global Header
	nSize = sizeof(pcap_hdr_t);
	nRet = fread(&stPcapHdr, (size_t)1, (size_t)nSize, fp);
	if(nRet < nSize)
	{
		printf("fread(%d).pcap_hdr_t = %d\n", nSize, nRet);
		return -1;
	}
	printf("PCAP.hdr, magic=[0x%x] version[%d.%d] thiszone=%d, sigfigs=%d, snaplen=%d, network=%d\n", 
		stPcapHdr.magic_number, stPcapHdr.version_major, stPcapHdr.version_minor, stPcapHdr.sigfigs, 
		stPcapHdr.thiszone, stPcapHdr.snaplen, stPcapHdr.network);

	#define LINKTYPE_ETHERNET	1
	#define LINKTYPE_LINUX_SLL	113
        switch (stPcapHdr.network)
        {
                case LINKTYPE_ETHERNET:
                        nSkipHdr = 14 + 40;
                        break;
                case LINKTYPE_LINUX_SLL:
                        nSkipHdr = 16 + 40;
                        break;
                default:
                        printf("unsupported link type=%x\n", stPcapHdr.network);
                        return -1;
        }
	
	if(nAmrCodec == 0)
	{
		amrnb_decode_init();
	}
	else
	{
		amrwb_decode_init();
	}

	while(1)
	{
		// Read PCAP Record Header
		nSize = sizeof(pcaprec_hdr_t);
		nRet = fread(&stRecHdr, (size_t)1, (size_t)nSize, fp);
		if(nRet < nSize)
		{
			printf("fread(%d).pcaprec_hdr_t = %d\n", nSize, nRet);
			break;
		}
		printf("PCAP.Rec[%03d], timestamp[%d.%d] incl=%d orig=%d\n", 
			nCount, stRecHdr.ts_sec, stRecHdr.ts_usec, stRecHdr.incl_len,  stRecHdr.orig_len);

		memset(&cData, 0, sizeof(cData));
		nRet = fread(cData, (size_t)1, (size_t)stRecHdr.incl_len, fp);
		printf("PCAP.data(%d).read=%d\n", stRecHdr.incl_len, nRet);

		if(nRet > nSkipHdr)
		{	// RTP 
			nSize = nRet - nSkipHdr;
			//nRet = fwrite(cData + nSkipHdr, (size_t)1, nSize, fout);
			printf("PCAP.amr_decode(0x%x, %d)............\n", (unsigned)(cData + nSkipHdr), nSize);
			if(nAmrCodec == 0)
			{
				nRet = amrnb_decode((char *)(cData + nSkipHdr), nSize, fout);
			}
			else
			{
				nRet = amrwb_decode((char *)(cData + nSkipHdr), nSize, fout);
			}
			printf("PCAP.amr_decode(0x%x, %d)=%d\n", (unsigned)(cData + nSkipHdr), nSize, nRet);
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
