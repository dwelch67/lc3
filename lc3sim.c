
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMMASK 0xFFFF
unsigned short mem[MEMMASK+1];
unsigned short reg[8];
unsigned short pc;
unsigned short psr;

unsigned short sext5 ( unsigned short x )
{
    if(x&0x10) x|=0xFFE0;
    return(x);
}
unsigned short sext6 ( unsigned short x )
{
    if(x&0x20) x|=0xFFC0;
    return(x);
}
unsigned short sext9 ( unsigned short x )
{
    if(x&0x100) x|=0xFE00;
    return(x);
}
unsigned short sext11 ( unsigned short x )
{
    if(x&0x0400) x|=0xF800;
    return(x);
}
void docc ( unsigned short x)
{
    psr&=0x8000;
    if(x&0x8000)
    {
        psr|=(1<<2);
    }
    else
    {
        if(x==0)
        {
            psr|=(1<<1);
        }
        else
        {
            psr|=(1<<0);
        }
    }
}

int sim ( void )
{
    unsigned short inst;
    unsigned short test;
    while(1)
    {
        inst=mem[pc]; pc=(pc+1)&MEMMASK;
        switch((inst>>12)&0xF)
        {
            case 0x0: //br
            {
                test=(psr&7)&((inst>>9)&7);
                if(test)
                {
                    pc=pc+sext9(inst&0x1FF);
                }
                break;
            }
            case 0x1: //add
            {
                if(inst&0x20)
                {
                    reg[(inst>>9)&7]=reg[(inst>>6)&7]+sext5(inst&0x1F);
                }
                else
                {
                    reg[(inst>>9)&7]=reg[(inst>>6)&7]+reg[(inst>>0)&7];
                }
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0x2: //ld
            {
                reg[(inst>>9)&7]=mem[pc+sext9(inst&0x1FF)];
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0x3: //st
            {
                mem[pc+sext9(inst&0x1FF)]=reg[(inst>>9)&7];
                break;
            }
            case 0x4: //jsr/jsrr
            {
                reg[7]=pc;
                if(inst&0x0800)
                {
                    pc=pc+sext11(inst&0x07FF);
                }
                else
                {
                    pc=reg[(inst>>6)&7];
                }
                break;
            }
            case 0x5: //and
            {
                if(inst&0x20)
                {
                    reg[(inst>>9)&7]=reg[(inst>>6)&7]&sext5(inst&0x1F);
                }
                else
                {
                    reg[(inst>>9)&7]=reg[(inst>>6)&7]&reg[(inst>>0)&7];
                }
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0x6: //ldr
            {
                reg[(inst>>9)&7]=mem[reg[(inst>>6)&7]+sext6(inst&0x3F)];
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0x7: //str
            {
                mem[reg[(inst>>6)&7]+sext6(inst&0x3F)]=reg[(inst>>9)&7];
                break;
            }
            case 0x8: //rti
            {
                if(psr&0x8000)
                {
                    printf("exception\n");
                    exit(1);
                }
                else
                {
                    pc=mem[reg[6]++];
                    psr=mem[reg[6]++];
                }
                break;
            }
            case 0x9: //not
            {
                reg[(inst>>9)&7]=~reg[(inst>>6)&7];
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0xA: //ldi
            {
                reg[(inst>>9)&7]=mem[mem[pc+sext9(inst&0x1FF)]];
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0xB: //sti
            {
                mem[mem[pc+sext9(inst&0x1FF)]]=reg[(inst>>9)&7];
                break;
            }
            case 0xC: //jmp/ret
            {
                pc=reg[(inst>>6)&7];
                break;
            }
            case 0xD: //undefined
            {
                printf("undefined instruction\n");
                exit(1);
                break;
            }
            case 0xE: //lea
            {
                reg[(inst>>9)&7]=pc+sext9(inst&0x1FF);
                docc(reg[(inst>>9)&7]);
                break;
            }
            case 0xF: //trap
            {
                if((inst&0xFF)==0x67)
                {
                    printf("show 0x%04X\n",reg[0]);
                }
                else
                if((inst&0xFF)==0x68)
                {
                    printf("psr 0x%04X\n",psr);
                }
                else
                if((inst&0xFF)==0x25)
                {
                    printf("halt\n");
                    return(0);
                }
                else
                {
                    reg[7]=pc;
                    pc=mem[inst&0xFF];
                }
                break;
            }
        }
    }
}
void reset ( void )
{
    pc=0x3000;
    psr=2;
    reg[0]=0x1234;
    reg[1]=0x1234;
    reg[2]=0x1234;
    reg[3]=0x1234;
    reg[4]=0x1234;
    reg[5]=0x1234;
    reg[6]=0x1234;
    reg[7]=0x1234;
}
//-----------------------------------------------------------------------------
void load_word ( unsigned int add, unsigned char hi, unsigned char lo )
{
    unsigned short data;
    //printf("0x%04X 0x%02X%02X\n",add,hi,lo);
    data=hi; data<<=8; data|=lo;
    mem[add&MEMMASK]=data;
}
//-----------------------------------------------------------------------------
extern int readhex ( FILE *fp );
int main ( int argc, char *argv[] )
{
    FILE *fp;

    if(argc<2)
    {
        fprintf(stderr,".hex file not specified\n");
        return(1);
    }

    memset(mem,0xDD,sizeof(mem));
    reset();

    fp=fopen(argv[1],"rt");
    if(fp==NULL)
    {
        fprintf(stderr,"Error opening file [%s]\n",argv[1]);
        return(1);
    }
    if(readhex(fp)) return(1);
    fclose(fp);

    sim();
    return(0);
}
