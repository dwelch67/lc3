
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAMMASK 0xFFFF
unsigned short mem[RAMMASK+1];
unsigned char mem_hit[RAMMASK+1];

//-----------------------------------------------------------------------------
void load_word ( unsigned int add, unsigned char hi, unsigned char lo )
{
    unsigned short data;

    //printf("0x%04X 0x%02X%02X\n",add,hi,lo);
    data=hi; data<<=8; data|=lo;
    mem[add&RAMMASK]=data;
    mem_hit[add&RAMMASK]++;
}
//-----------------------------------------------------------------------------
int readhex ( FILE *fp )
{

    char gstring[80];
    char newline[1024];
    unsigned char hexline[1024];

//#define RAMMASK 0xFFFF


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

unsigned short sext5 ( unsigned short x )
{
    x&=0x001F;
    if(x&0x10) x|=0xFFE0;
    return(x);
}
unsigned short sext6 ( unsigned short x )
{
    x&=0x003F;
    if(x&0x20) x|=0xFFC0;
    return(x);
}
unsigned short sext9 ( unsigned short x )
{
    x&=0x01FF;
    if(x&0x100) x|=0xFE00;
    return(x);
}
unsigned short sext11 ( unsigned short x )
{
    x&=0x07FF;
    if(x&0x0400) x|=0xF800;
    return(x);
}

int main ( void )
{
    unsigned int ra;
    unsigned int addr;
    unsigned int inst;
    unsigned int rd,rs1,rs2,simm,dest;
    FILE *fp;

    memset(mem,0xFF,sizeof(mem));
    memset(mem_hit,0x00,sizeof(mem_hit));

    fp=fopen("test.asm.hex","rt");
    if(fp==NULL) return(1);
    if(readhex(fp)) return(1);

    for(addr=0;addr<=RAMMASK;addr++)
    {
        if(mem_hit[addr])
        {
            inst=mem[addr];
            printf("0x%04X: 0x%04X ",addr,inst);
            switch(inst&0xF000)
            {
                case 0x0000:
                {
                    printf("br");
                    if(inst&0x0800) printf("n");
                    if(inst&0x0400) printf("z");
                    if(inst&0x0200) printf("p");
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf(" 0x%04X",dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x1000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    if(inst&0x20)
                    {
                        simm=sext5(inst);
                        printf("add r%u,r%u,#0x%04X",rd,rs1,simm);
                    }
                    else
                    {
                        rs2=(inst>>0)&7;
                        printf("add r%u,r%u,r%u",rd,rs1,rs2);
                    }
                    break;
                }
                case 0x2000:
                {
                    rd=(inst>>9)&7;
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("ld r%u,0x%04X",rd,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x3000:
                {
                    rd=(inst>>9)&7;
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("st r%u,0x%04X",rd,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x4000:
                {
                    if(inst&0x0800)
                    {
                        simm=sext11(inst);
                        dest=(addr+1+simm)&0xFFFF;
                        printf("jsr 0x%04X",dest);
                        if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                        else            printf(" ; +%u",simm);
                    }
                    else
                    {
                        rd=(inst>>6)&7;
                        printf("jsrr r%u",rd);
                    }
                    break;
                }
                case 0x5000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    if(inst&0x20)
                    {
                        simm=sext5(inst);
                        printf("and r%u,r%u,#0x%04X",rd,rs1,simm);
                    }
                    else
                    {
                        rs2=(inst>>0)&7;
                        printf("and r%u,r%u,r%u",rd,rs1,rs2);
                    }
                    break;
                }
                case 0x6000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    simm=sext6(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("ldr r%u,r%u,0x%04X",rd,rs1,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x7000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    simm=sext6(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("str r%u,r%u,0x%04X",rd,rs1,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x8000:
                {
                    printf("rti");
                    break;
                }
                case 0x9000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    printf("not r%u,r%u",rd,rs1);
                    break;
                }
                case 0xA000:
                {
                    rd=(inst>>9)&7;
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("ldi r%u,0x%04X",rd,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0xB000:
                {
                    rd=(inst>>9)&7;
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("sti r%u,0x%04X",rd,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0xC000:
                {
                    printf("ret");
                    break;
                }
                case 0xD000:
                {
                    printf("undefined");
                    break;
                }
                case 0xE000:
                {
                    rd=(inst>>9)&7;
                    simm=sext9(inst);
                    dest=(addr+1+simm)&0xFFFF;
                    printf("lea r%u,0x%04X",rd,dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0xF000:
                {
                    rd=inst&0xFF;
                    printf("trap 0x%02X",rd);
                    break;
                }
            }


            printf("\n");
        }
    }



    return(0);
}


