
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-----------------------------------------------------------------------------
extern void load_word ( unsigned int add, unsigned char hi, unsigned char lo );
//-----------------------------------------------------------------------------
int readhex ( FILE *fp )
{

    char gstring[80];
    char newline[1024];
    unsigned char hexline[1024];

#define RAMMASK 0xFFFF


    //unsigned int errors;

    //unsigned int addhigh;
    unsigned int add;
    unsigned int data;

    unsigned int ra;
    //unsigned int rb;

    //unsigned int pages;
    //unsigned int page;

    unsigned int line;

    unsigned char checksum;

    unsigned int len;
    unsigned int hexlen;
    unsigned int maxadd;

    unsigned char t;

    maxadd=0;

    //addhigh=0;

    line=0;
    while(fgets(newline,sizeof(newline)-1,fp))
    {
        line++;
        //printf("%s",newline);
        if(newline[0]!=':')
        {
            printf("Syntax error <%u> no colon\n",line);
            continue;
        }
        gstring[0]=newline[1];
        gstring[1]=newline[2];
        gstring[2]=0;
        len=strtoul(gstring,NULL,16);
        for(ra=0;ra<(len+5);ra++)
        {
            gstring[0]=newline[(ra<<1)+1];
            gstring[1]=newline[(ra<<1)+2];
            gstring[2]=0;
            hexline[ra]=(unsigned char)strtoul(gstring,NULL,16);
        }
        checksum=0;
        for(ra=0;ra<(len+5);ra++) checksum+=hexline[ra];
        //checksum&=0xFF;
        if(checksum)
        {
            printf("checksum error <%u>\n",line);
        }
        add=hexline[1]; add<<=8;
        add|=hexline[2];
        //add|=addhigh;
        if(add>RAMMASK)
        {
            printf("address too big 0x%08X\n",add);
            //return(1);
            continue;
        }
        t=hexline[3];
        if(t!=0x02)
        {
            if(len&1)
            {
                printf("bad length\n");
                return(1);
            }
        }

        //:0011223344
        //;llaaaattdddddd
        //01234567890
        switch(t)
        {
            case 0x00:
            {
                for(ra=0;ra<len;ra+=4)
                {
                    if(add>RAMMASK)
                    {
                        printf("address too big 0x%08X\n",add);
                        break;
                    }
                    load_word(add++,hexline[ra+4+0],hexline[ra+4+1]);
                    if(add>maxadd) maxadd=add;
                }
                break;
            }
            case 0x01:
            {
//                printf("End of data\n");
                break;
            }
            default:
            {
                printf("UNKNOWN type %02X <%u>\n",t,line);
                break;
            }
        }
        if(t==0x01) break; //end of data
    }

    //printf("%u lines processed\n",line);
    //printf("%08X\n",maxadd);

    //for(ra=0;ra<maxadd;ra+=4)
    //{
        //printf("0x%08X: 0x%08X\n",ra,ram[ra>>2]);
    //}

    return(0);
}
