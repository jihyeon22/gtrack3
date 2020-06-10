#include <stdio.h>
#include <string.h>

#include <unistd.h> 
#include <time.h>

#include "leakdata-list.h"
#include "packetdata_util.h"

#include <at/at_util.h>
#include "kt_flood_mdt800/packet.h"

char g_uniqueID[5] = {0x00,};

int hex2bin( const char *s )
{
    int ret=0;
    int i;
    for( i=0; i<2; i++ )
    {
        char c = *s++;
        int n=0;
        if( '0'<=c && c<='9' )
            n = c-'0';
        else if( 'a'<=c && c<='f' )
            n = 10 + c-'a';
        else if( 'A'<=c && c<='F' )
            n = 10 + c-'A';
        ret = n + ret*16;
    }
    return ret;
}

int getstr2number( const char *s )
{
    int ret=0;
    int i;
    for( i=0; i<2; i++ )
    {
        char c = *s++;
        int n=0;
        if( '0'<=c && c<='9' )
            n = c-'0';
        else if( 'a'<=c && c<='f' )
            n = 10 + c-'a';
        else if( 'A'<=c && c<='F' )
            n = 10 + c-'A';
        ret = n + ret*10;
    }
    return ret;
}

int get_phonenum_binary(char *out)
{
    char *in;
    char phonenum[MAX_DEV_ID_LED];
    int i;

    at_get_phonenum(phonenum, MAX_DEV_ID_LED);

    printf("%d\n", strlen(g_uniqueID));   
    
    if(strlen(g_uniqueID) == 5)
    {
        memcpy(out,g_uniqueID, strlen(g_uniqueID));
        printf("g_uniqueID strlen : %d \n", strlen(g_uniqueID));

    }

    in = &phonenum[1];
    for( i=0; i<5; i++ )
    {
        out[i] = getstr2number(in);
        in += 2;        
    }
    sprintf(g_uniqueID, "%s", out);
    //printf("%s\n", g_uniqueID);


    return i;
}
int to_bigedian(int bits32)
{
    unsigned char bytes[4];
    int ret;

    bytes[0] = (unsigned char)((bits32 >> 0) & 0xff);
    bytes[1] = (unsigned char)((bits32 >> 8) & 0xff);
    bytes[2] = (unsigned char)((bits32 >> 16) & 0xff);
    bytes[3] = (unsigned char)((bits32 >> 24) & 0xff);

    ret =   ((int) bytes[0] << 24) |
            ((int) bytes[1] << 16) |
            ((int) bytes[2] << 8) |
            ((int) bytes[3] << 0);

    return ret;
}