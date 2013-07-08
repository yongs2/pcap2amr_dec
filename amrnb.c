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
void *enc1 = NULL;
extern int b_is_rtp;
extern int b_octet_align;
extern int dtx;
extern int mode;
extern int ptime;

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
	dec1 = NULL;
	return 0;
}

int amrnb_read_bytes(int nMode)
{
	if( (nMode >= 0) && (nMode < MAX_FRAME_TYPE) )
	{
		return amr_frame_sizes[nMode];
	}
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
	{
		printf("Too short packet\n");
		return -1;
	}
	
	payload = bs_new((uint8_t *)pData, nSize);
	if(payload == NULL)
	{
		return -2;
	}

	//printf("%s, Size=%d, bs_new.p[5]{%02x %02x%02x %02x%02x}, left=%d\n", __func__, nSize, 
	//	payload->p[0], payload->p[1], payload->p[2], payload->p[3], payload->p[4], payload->bits_left);
	if(b_is_rtp == 1)
	{
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
			printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, MAX_FRAME_TYPE);
			break;
		}
		nFrameData += amr_frame_sizes[nFTbits];
		nQbit = bs_read_u(payload, 1);
		tocs[nTocLen++] = ((nFbit << 7) | (nFTbits << 3) | (nQbit << 2)) & 0xFC;
		if(b_octet_align == 1)
		{	// octet-align 모드에서는 Padding bit 2bit를 더 읽어야 한다.
			nPadding = bs_read_u(payload, 2);
		}
		//printf("%s, F=%d, FT=%d, Q=%d, tocs[%d]=0x%x, FrameData=%d\n", __func__, nFbit, nFTbits, nQbit, nTocLen, tocs[nTocLen-1], nFrameData);
	} // end of while
	nBitLeft = payload->bits_left;
	
	if(b_octet_align == 0)
	{
		//printf("%s, nCmr=%d, TOC=%d, nPadding(%d)=%d, FrameData=%d\n", __func__, nCmr, nTocLen, nBitLeft, nPadding, nFrameData);
	}
	else
	{
		//printf("%s, nCmr=%d, nReserved=%d, TOC=%d, nPadding(%d)=%d, FrameData=%d\n", __func__, nCmr, nReserved, nTocLen, nBitLeft, nPadding, nFrameData);
	}
	
	toclen = toc_list_check(tocs, nSize);
	if (toclen == -1)
	{
		printf("Bad AMR toc list\n");
		bs_free(payload);
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
			printf("Bad amr toc, index=%i\n", index);
			break;
		}
		framesz = amr_frame_sizes[index];
		nRead = bs_read_bytes_ex(payload, &tmp[1], framesz);
		//printf("%s, toc=0x%x, bs_read_bytes_ex(framesz=%d)=%d,tmp[5]{%02x %02x%02x %02x%02x}\n", __func__, tmp[0], framesz, nRead, tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
		nSize = nsamples * 2;
		
		Decoder_Interface_Decode(dec1, tmp, (short*) output, 0);
		nRet = fwrite(output, (size_t)1, nSize, fp);
	} // end of for
	bs_free(payload);

	return nRet;
}

int amrnb_encode_init()
{
	enc1 = Encoder_Interface_init(dtx);
	return 0;
}

int amrnb_encode_uninit()
{
	Encoder_Interface_exit(enc1);
	enc1 = NULL;
	return 0;
}

int amrnb_encode(char *pData, int nSize, FILE *fp)
{
	int nRet = 0;
	unsigned int unitary_buff_size = sizeof (int16_t) * NUM_SAMPLES;
	unsigned int buff_size = unitary_buff_size * ptime / 20;
	uint8_t tmp[OUT_MAX_SIZE];
	int16_t samples[buff_size];
	uint8_t	tmp1[20*OUT_MAX_SIZE];
	bs_t	*payload = NULL;
	int		nCmr = 0xF;
	int		nFbit = 1, nFTbits = 0, nQbit = 0;
	int		nReserved = 0, nPadding = 0;
	int		nFrameData = 0, framesz = 0, nWrite = 0;
	int		offset = 0;

	uint8_t output[OUT_MAX_SIZE * buff_size / unitary_buff_size + 1];
	int 	nOutputSize = 0;

	printf("%s, unitary_buff_size=%d, buff_size=%d, sizeof(output)=%lu\n", __func__, unitary_buff_size, buff_size, sizeof(output));

	while (nSize >= buff_size)
	{
		memset(output, 0, sizeof(output));
		memcpy((uint8_t*)samples, pData, buff_size);
		payload = bs_new(output, OUT_MAX_SIZE * buff_size / unitary_buff_size + 1);
		
		if(b_is_rtp == 1)
		{
			/** AMR file 로 생성할 때에는 CMR 정보를 저장할 필요가 없다. RTP 로 전송할 때에만 필요함
			*/
			if(b_octet_align == 0)
			{	// Bandwidth efficient mode
				// 1111 ; CMR (4 bits)
				bs_write_u(payload, 4, nCmr);
			}
			else
			{	// octet-aligned mode
				// 1111 0000 ; CMR (4 bits), Reserved (4 bits)
				bs_write_u(payload, 4, nCmr);
				bs_write_u(payload, 4, nReserved);
			}
		}
		
		nFrameData = 0; nWrite = 0;
		for (offset = 0; offset < buff_size; offset += unitary_buff_size)
		{
			int ret = Encoder_Interface_Encode(enc1, mode, &samples[offset / sizeof (int16_t)], tmp, dtx);
			if (ret <= 0 || ret > 32)
			{
				printf("Encoder returned %i\n", ret);
				continue;
			}
			nFbit = tmp[0] >> 7;
			nFbit = (offset+buff_size >= unitary_buff_size) ? 0 : 1;
			nFTbits = tmp[0] >> 3 & 0x0F;
			if(nFTbits > MAX_FRAME_TYPE)
			{
				printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, MAX_FRAME_TYPE);
				break;
			}
			nQbit = tmp[0] >> 2 & 0x01;
			framesz = amr_frame_sizes[nFTbits];
			printf("%s, %03d(%d,%d), F=%d, FT=%d, Q=%d, framesz=%d (%d),tmp1=%d\n", __func__, offset, offset+buff_size, unitary_buff_size, nFbit, nFTbits, nQbit, framesz, ret-1, nFrameData);
			
			// Frame 데이터를 임시로 복사
			memcpy(&tmp1[nFrameData], &tmp[1], framesz);
			nFrameData += framesz;
			
			// write TOC
			bs_write_u(payload, 1, nFbit);
			bs_write_u(payload, 4, nFTbits);
			bs_write_u(payload, 1, nQbit);
			if(b_octet_align == 1)
			{	// octet-align, add padding bit
				bs_write_u(payload, 2, nPadding);
			}
		} // end of for
		if(offset > 0)
		{
			nWrite = bs_write_bytes_ex(payload, tmp1, nFrameData);
		}
		printf("%s, bs_write_bytes_ex(framesz=%d)=%d(%d),tmp[6]{%02x%02x %02x%02x %02x%02x}\n", __func__, framesz, nWrite, bs_pos(payload), tmp1[0], tmp1[1], tmp1[2], tmp1[3], tmp1[4], tmp1[5]);
		nOutputSize = 1 + framesz;

		nRet = fwrite(output, (size_t)1, nOutputSize, fp);
		printf("%s, fwrite(%d)=%d\n", __func__, nOutputSize, nRet);
		
		bs_free(payload);

		nSize -= buff_size;
	} // end of while
	return nRet;
}
