
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMMASK 0xFFFF
unsigned short mem[MEMMASK+1];
unsigned short reg[8];
unsigned short pc;
unsigned short psr;

unsigned int fetch_mem_count;
unsigned int read_mem_count;
unsigned int write_mem_count;


unsigned short fetch_mem ( unsigned short add )
{
    unsigned short data;
    fetch_mem_count++;
    data=mem[add];
    printf("fetch_mem[0x%04X]=>0x%04X\n",add,data);
    return(data);
}
unsigned short read_mem ( unsigned short add )
{
    unsigned short data;
    read_mem_count++;
    data=mem[add];
    printf("read_mem[0x%04X]=>0x%04X\n",add,data);
    return(data);
}
void write_mem ( unsigned short add, unsigned short data )
{
    write_mem_count++;
    printf("write_mem[0x%04X]<=0x%04X\n",add,data);
}
unsigned short read_reg ( unsigned short r )
{
    unsigned short d;
    r&=7;
    d=reg[r];
    printf("read_reg[%u]=>0x%04X\n",r,d);
    return(d);
}
void write_reg ( unsigned short r, unsigned short d )
{
    r&=7;
    reg[r]=d;
    printf("write_reg[%u]<=0x%04X\n",r,d);
}

unsigned short sext5 ( unsigned short x )
{
    x&=0x1F;
    if(x&0x10) x|=0xFFE0;
    return(x);
}
unsigned short sext6 ( unsigned short x )
{
    x&=0x3F;
    if(x&0x20) x|=0xFFC0;
    return(x);
}
unsigned short sext9 ( unsigned short x )
{
    x&=0x1FF;
    if(x&0x100) x|=0xFE00;
    return(x);
}
unsigned short sext11 ( unsigned short x )
{
    x&=0x7FF;
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
    printf("psr 0x%04X\n",psr);
}

int sim ( void )
{
    unsigned short inst;
    unsigned short test;
    unsigned short simm;
    unsigned short dest;
    unsigned short rd,rd_data;
    unsigned short sr1,sr1_data;
    unsigned short sr2,sr2_data;
    while(1)
    {
        printf("\n");
        inst=fetch_mem(pc);
        printf("---- 0x%04X: 0x%04X ",pc,inst);
        pc=(pc+1)&MEMMASK;
        switch((inst>>12)&0xF)
        {
            case 0x0: //br
            {
                printf("br");
                if(inst&0x0800) printf("n");
                if(inst&0x0400) printf("z");
                if(inst&0x0200) printf("p");
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf(" 0x%04X",dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");
                test=(psr&7)&((inst>>9)&7);
                if(test)
                {
                    pc=pc+sext9(inst&0x1FF);
                }
                break;
            }
            case 0x1: //add
            {
                rd=(inst>>9)&7;
                sr1=(inst>>6)&7;
                if(inst&0x20)
                {
                    simm=sext5(inst);
                    printf("add r%u,r%u,#0x%04X\n",rd,sr1,simm);

                    sr1_data=read_reg(sr1);
                    rd_data=sr1_data+simm;
                }
                else
                {
                    sr2=(inst>>0)&7;
                    printf("add r%u,r%u,r%u",rd,sr1,sr2);

                    sr1_data=read_reg(sr1);
                    sr2_data=read_reg(sr2);
                    rd_data=sr1_data+sr2_data;
                }
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0x2: //ld
            {
                rd=(inst>>9)&7;
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf("ld r%u,0x%04X",rd,dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                rd_data=read_mem(dest);
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0x3: //st
            {
                rd=(inst>>9)&7;
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf("st r%u,0x%04X",rd,dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                rd_data=read_reg(rd);
                write_mem(dest,rd_data);
                break;
            }
            case 0x4: //jsr/jsrr
            {
                if(inst&0x0800)
                {
                    simm=sext11(inst);
                    dest=(pc+simm)&0xFFFF;
                    printf("jsr 0x%04X",dest);
                    if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                    else            printf(" ; +%u",simm);
                    printf("\n");

                    write_reg(7,pc);
                    pc=dest;
                }
                else
                {
                    rd=(inst>>6)&7;
                    printf("jsrr r%u\n",rd);

                    write_reg(7,pc);
                    pc=read_reg(rd);
                }
                break;
            }
            case 0x5: //and
            {
                rd=(inst>>9)&7;
                sr1=(inst>>6)&7;
                if(inst&0x20)
                {
                    simm=sext5(inst);
                    printf("and r%u,r%u,#0x%04X\n",rd,sr1,simm);

                    sr1_data=read_reg(sr1);
                    rd_data=sr1_data&simm;
                }
                else
                {
                    sr2=(inst>>0)&7;
                    printf("add r%u,r%u,r%u",rd,sr1,sr2);

                    sr1_data=read_reg(sr1);
                    sr2_data=read_reg(sr2);
                    rd_data=sr1_data&sr2_data;
                }
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0x6: //ldr
            {
                rd=(inst>>9)&7;
                sr1=(inst>>6)&7;
                simm=sext6(inst);
                printf("ldr r%u,r%u,#0x%04X",rd,sr1,simm);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                sr1_data=read_reg(sr1);
                dest=(sr1_data+simm)&0xFFFF;
                rd_data=read_mem(dest);
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0x7: //str
            {
                rd=(inst>>9)&7;
                sr1=(inst>>6)&7;
                simm=sext6(inst);
                printf("str r%u,r%u,#0x%04X",rd,sr1,simm);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                sr1_data=read_reg(sr1);
                dest=(sr1_data+simm)&0xFFFF;
                rd_data=read_reg(rd);
                write_mem(dest,rd_data);
                break;
            }
            case 0x8: //rti
            {
                printf("rti\n");
                if(psr&0x8000)
                {
                    printf("exception\n");
                    exit(1);
                }
                else
                {
                    rd_data=read_reg(6);
                    pc=read_mem(rd_data);
                    write_reg(6,(rd_data+1)&0xFFFF);
                    rd_data=read_reg(6);
                    psr=read_mem(rd_data);
                    write_reg(6,(rd_data+1)&0xFFFF);
                }
                break;
            }
            case 0x9: //not
            {
                rd=(inst>>9)&7;
                sr1=(inst>>6)&7;
                printf("not r%u,r%u\n",rd,sr1);

                sr1_data=read_reg(sr1);
                rd_data = ~sr1_data;
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0xA: //ldi
            {
                rd=(inst>>9)&7;
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf("ldi r%u,0x%04X",rd,dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                rd_data=read_mem(read_mem(dest));
                write_reg(rd,rd_data);
                docc(rd_data);
                break;
            }
            case 0xB: //sti
            {
                rd=(inst>>9)&7;
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf("st r%u,0x%04X",rd,dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                rd_data=read_reg(rd);
                write_mem(read_mem(dest),rd_data);
                break;
            }
            case 0xC: //jmp/ret
            {
                sr1=(inst>>6)&7;
                printf("jmp r%u",sr1);
                if(sr1==7) printf(" ; ret");
                printf("\n");

                pc=read_reg(sr1);
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
                rd=(inst>>9)&7;
                simm=sext9(inst);
                dest=(pc+simm)&0xFFFF;
                printf("lea r%u,0x%04X",rd,dest);
                if(simm&0x8000) printf(" ; -%u",(simm^0xFFFF)+1);
                else            printf(" ; +%u",simm);
                printf("\n");

                write_reg(rd,dest);
                docc(dest);
                break;
            }
            case 0xF: //trap
            {
                printf("trap 0x%02X\n",inst&0xFF);
                if((inst&0xFF)==0x67)
                {
                    printf("show 0x%04X\n",reg[0]);
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
    psr=0;
    reg[0]=0;
    reg[1]=0;
    reg[2]=0;
    reg[3]=0;
    reg[4]=0;
    reg[5]=0;
    reg[6]=0;
    reg[7]=0;
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
