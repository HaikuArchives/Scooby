#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};
#define CHAR64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

ssize_t encode64(char *out,char *_in, unsigned inlen)
{
    unsigned char oval;
 	static char basis64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 	int i;
 	char *in = malloc(inlen+1);
 	memcpy(in,_in,inlen);
 	in[inlen] = '\0';
 	
 	i =0;
 	for (; inlen >= 3; inlen -= 3){
        *out++ = basis64[in[i+0] >> 2];
        *out++ = basis64[((in[i+0] << 4) & 0x30) | (in[i+1] >> 4)];
        *out++ = basis64[((in[i+1] << 2) & 0x3c) | (in[i+2] >> 6)];
        *out++ = basis64[in[i+2] & 0x3f];
        i += 3;
    }
    if (inlen > 0) {
        *out++ = basis64[in[i+0] >> 2];
        oval = (in[i+0] << 4) & 0x30;
        if (inlen > 1) oval |= in[i+1] >> 4;
        *out++ = basis64[oval];
        *out++ = (inlen < 2) ? '=' : basis64[(in[i+1] << 2) & 0x3c];
        *out++ = '=';
    }
    *out = '\0';
    free(in);
    return strlen((const char*)out);
}


ssize_t decode64(char *out,const char *_in, unsigned inlen)
{
    unsigned len = 0,lup;
    int c1, c2, c3, c4;
	int i,iR;
	char *in = malloc(inlen+1);
 	memcpy(in,_in,inlen);
 	in[inlen] = '\0';
 	
 	i = 0;
    iR=0;
    if (in[i+0] == '+' && in[i+1] == ' ') i += 2;
	
    for (lup=0;lup<inlen/4;lup++)
    {
    	if(in[i] == '\r'|| in[i] == '\n') 
        { 
            i++; 
        	continue; 
        } 
        c1 = in[i+0];
        c2 = in[i+1];
        c3 = in[i+2];
        c4 = in[i+3];
        i += 4;
        out[iR++] = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
        ++len;
        if (c3 != '=')
        {
            out[iR++] = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
            ++len;
            if (c4 != '=')
            {
                out[iR++] = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
                ++len;
            }
        }
    }

    out[iR]='\0'; /* terminate string */
 	free(in);
 	return strlen(out);
}