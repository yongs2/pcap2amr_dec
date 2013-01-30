#include <stdio.h>
#include <stdlib.h>
#include <opencore-amrnb/interf_dec.h>
#include <opencore-amrnb/interf_enc.h>
#include "bs.h"

#define MAX_FRAME_TYPE	(8)		// SID Packet
#define OUT_MAX_SIZE	(32)
#define NUM_SAMPLES 	(160)

static const int amr_frame_rates[] = {4750, 5150, 5900, 6700, 7400, 7950, 10200, 12200};

static const int amr_frame_sizes[] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0 };

void *dec1 = NULL;
extern int b_octet_align;

#define toc_get_f(toc) ((toc) >> 7)
#define toc_get_index(toc)	((toc>>3) & 0xf)

static int toc_list_check(uint8_t *tl, size_t buflen) 
{
	int s = 1;
	while (toc_get_f(*tl))
	{
		tl++;
		s++;
		if (s > buflen)
		{
			return -1;
		}
	}
	return s;
}

int amrnb_decode_init()
{
	dec1 = Decoder_Interface_init();
	return 0;
}

int amrnb_decode_uninit()
{
	Decoder_Interface_exit(dec1);
	return 0;
}

int amrnb_decode(char *pData, int nSize, FILE *fp)
{
	int nRet = 0;
	static const int nsamples = NUM_SAMPLES;
	uint8_t tmp[OUT_MAX_SIZE];
	uint8_t output[NUM_SAMPLES * 2];

	uint8_t	tocs[20] = {0,};
	int 	nTocLen = 0, toclen = 0;
	bs_t	*payload = NULL;
	int		nCmr = 0, nBitLeft = 0, nPadding = 0, nReserved = 0, nRead = 0;
	int		nFbit = 1;
	int		nFTbits = 0;
	int		nQbit = 0;
	int		nFrameData = 0;
	int		i = 0, index = 0, framesz = 0;

	if(nSize < 2)
		return -1;

	payload = bs_new((uint8_t *)pData, nSize);
	if(payload == NULL)
		return -2;

	printf("%s, Size=%d, bs_new.p[5]{%02x %02x%02x %02x%02x}, left=%d\n", __func__, nSize, 
		payload->p[0], payload->p[1], payload->p[2], payload->p[3], payload->p[4], payload->bits_left);

	if(b_octet_align == 0)
	{	// Bandwidth efficient mode
		// 1111 ; CMR (4 bits)
		nCmr = bs_read_u(payload, 4);
	}
	else
	{	// octet-aligned mode
		// 1111 0000 ; CMR (4 bits), Reserved (4 bits)
		nCmr = bs_read_u(payload, 4);
		nReserved = bs_read_u(payload, 4);
	}
	nTocLen = 0; nFrameData = 0;
	while(nFbit == 1)
	{	// 0                   1
		// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		// |1|  FT   |Q|1|  FT   |Q|0|  FT   |Q|
		// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		nFbit = bs_read_u(payload, 1);
		nFTbits = bs_read_u(payload, 4);
		if(nFTbits > MAX_FRAME_TYPE)
		{
			printf("%s, Bad amr toc, index=%i (MAX=%d)", __func__, nFTbits, MAX_FRAME_TYPE);
			break;
		}
		nFrameData += amr_frame_sizes[nFTbits];
		nQbit = bs_read_u(payload, 1);
		tocs[nTocLen++] = ((nFbit << 7) | (nFTbits << 3) | (nQbit << 2)) & 0xFC;
		if(b_octet_align == 1)
		{	// octet-align 모드에서는 Padding bit 2bit를 더 읽어야 한다.
			nPadding = bs_read_u(payload, 2);
		}
		printf("%s, F=%d, FT=%d, Q=%d, tocs[%d]=0x%x, FrameData=%d\n", __func__, nFbit, nFTbits, nQbit, nTocLen, tocs[nTocLen-1], nFrameData);
	} // end of while
	nBitLeft = payload->bits_left;
	
	if(b_octet_align == 0)
	{
		printf("%s, nCmr=%d, TOC=%d, nPadding(%d)=%d, FrameData=%d\n", __func__, nCmr, nTocLen, nBitLeft, nPadding, nFrameData);
	}
	else
	{
		printf("%s, nCmr=%d, nReserved=%d, TOC=%d, nPadding(%d)=%d, FrameData=%d\n", __func__, nCmr, nReserved, nTocLen, nBitLeft, nPadding, nFrameData);
	}
	
	toclen = toc_list_check(tocs, nSize);
	if (toclen == -1)
	{
		printf("Bad AMR toc list");
		return -3;
	}
    
	if((nFrameData) != bs_bytes_left(payload))
	{
		printf("%s, invalid data mismatch, FrameData=%d, bytes_left=%d\n", __func__, nFrameData, bs_bytes_left(payload));
	}
	for(i=0; i<nTocLen; i++)
	{
		memset(tmp, 0, sizeof(tmp));
		tmp[0] = tocs[i];
		index = toc_get_index(tocs[i]);
		if (index > MAX_FRAME_TYPE)
		{
			printf("Bad amr toc, index=%i", index);
			break;
		}
		framesz = amr_frame_sizes[index];
		nRead = bs_read_bytes_ex(payload, &tmp[1], framesz);
		printf("%s, toc=0x%x, bs_read_bytes_ex(framesz=%d)=%d,tmp[5]{%02x %02x%02x %02x%02x}\n", __func__, tmp[0], framesz, nRead, tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
		nSize = nsamples * 2;
		Decoder_Interface_Decode(dec1, tmp, (short*) output, 0);
		nRet = fwrite(output, (size_t)1, nSize, fp);
	} // end of for
	bs_free(payload);

	return nRet;
}
