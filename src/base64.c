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

ssize_t encode64(char *out,char *_in, unsigned len)
{
 	static char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 	int i,r,tmp;
	unsigned char* p;
   
	if( len == 0 ){
        out[0] = '\0';
        return 0;
    }

    p = (unsigned char*) _in;
    for( r = i = 0; i < len; ){
        tmp = p[i++];
        tmp <<= 8;
        if( i < len ) tmp += p[i];
        tmp <<= 8;
        i++;
        if( i < len ) tmp += p[i];
        i++;

        out[r++] = base[(tmp & 0x00FC0000) >> 18];
        out[r++] = base[(tmp & 0x0003F000) >> 12];
        out[r++] = base[(tmp & 0x00000FC0) >> 6 ];
        out[r++] = base[(tmp & 0x0000003F) >> 0 ];
        
        if( i > len ) out[r-1] = '=';
        if( i > len+1 ) out[r-2] = '=';
    }

    out[r] = '\0';
	return r;
}


ssize_t decode64(char *out,const char *in, unsigned len)
{
    int i, n;
    char *p;
    int tmp;

    if( in == NULL || len < 0 ){
        return -1;
    }

 	if( len == 0 ){
        out[0] = '\0';
        return 0;
    }

    p = (char *)in;
    for( n = i = 0; i < len; i += 4){
        tmp = index_64[ (unsigned int)p[i] ];
        tmp *= 64;
        tmp += index_64[ (unsigned int)p[i+1] ];
        tmp *= 64;
        if( p[i+2] != '=' ) tmp += index_64[ (unsigned int)p[i+2] ];
        tmp *= 64;
        if( p[i+3] != '=' ) tmp += index_64[ (unsigned int)p[i+3] ];

        out[n++] = ( tmp & 0x00FF0000 ) >> 16;
        out[n++] = ( tmp & 0x0000FF00 ) >> 8;
        out[n++] = ( tmp & 0x000000FF ) >> 0;
    }

    return n-1;
}
