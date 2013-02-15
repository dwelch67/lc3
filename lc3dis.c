
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

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
extern int readhex ( FILE *fp );
//-----------------------------------------------------------------------------
unsigned short sext5 ( unsigned short x )
{
    x&=0x001F;
    if(x&0x10) x|=0xFFE0;
    return(x);
}
//-----------------------------------------------------------------------------
unsigned short sext6 ( unsigned short x )
{
    x&=0x003F;
    if(x&0x20) x|=0xFFC0;
    return(x);
}
//-----------------------------------------------------------------------------
unsigned short sext9 ( unsigned short x )
{
    x&=0x01FF;
    if(x&0x100) x|=0xFE00;
    return(x);
}
//-----------------------------------------------------------------------------
unsigned short sext11 ( unsigned short x )
{
    x&=0x07FF;
    if(x&0x0400) x|=0xF800;
    return(x);
}
//-----------------------------------------------------------------------------
int main ( int argc, char *argv[] )
{
    unsigned int ra;
    unsigned int addr;
    unsigned int inst;
    unsigned int rd,rs1,rs2,simm,dest;
    FILE *fp;

    if(argc<2)
    {
        fprintf(stderr,".hex file not specified\n");
        return(1);
    }

    memset(mem,0xFF,sizeof(mem));
    memset(mem_hit,0x00,sizeof(mem_hit));

    fp=fopen(argv[1],"rt");
    if(fp==NULL)
    {
        fprintf(stderr,"Error opening file [%s]\n",argv[1]);
        return(1);
    }
    if(readhex(fp)) return(1);
    fclose(fp);

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
                    printf("ldr r%u,r%u,#0x%04X",rd,rs1,simm);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    break;
                }
                case 0x7000:
                {
                    rd=(inst>>9)&7;
                    rs1=(inst>>6)&7;
                    simm=sext6(inst);
                    printf("str r%u,r%u,#0x%04X",rd,rs1,simm);
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
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
