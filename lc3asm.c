
//-------------------------------------------------------------------
// Copyright (C) 2012 David Welch
//-------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE *fpin;
FILE *fpout;

unsigned int rd,rs,rt,rx;
unsigned int is_const,is_label;

#define ADDMASK 0xFFFF
unsigned int mem[ADDMASK+1];
unsigned int mark[ADDMASK+1];

unsigned int curradd;
unsigned int line;

char cstring[1024];
char newline[1024];


#define LABLEN 64

#define MAX_LABS 65536
struct
{
    char name[LABLEN];
    unsigned int addr;
    unsigned int line;
    unsigned int type;
} lab_struct[MAX_LABS];
unsigned int nlabs;
unsigned int lab;


#define NREGNAMES (8)
struct
{
    char name[4];
    unsigned char num;
} reg_names[NREGNAMES]=
{
    {"r0",0},{"r1",1},{"r2",2},{"r3",3},{"r4",4},{"r5",5},{"r6",6},{"r7",7}
};

unsigned char hexchar[256]=
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

unsigned char numchar[256]=
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
int rest_of_line ( unsigned int ra )
{
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    if(newline[ra])
    {
        printf("<%u> Error: garbage at end of line\n",line);
        return(1);
    }
    return(0);
}
//-------------------------------------------------------------------
int get_reg_number ( char *s, unsigned int *rx )
{
    unsigned int ra;

    for(ra=0;ra<NREGNAMES;ra++)
    {
        if(strcmp(s,reg_names[ra].name)==0)
        {
            *rx=reg_names[ra].num;
            return(0);
        }
    }
    return(1);
}
//-------------------------------------------------------------------
unsigned int check_simmed5 ( unsigned int rx )
{
    rx&=0xFFFF;
    rx>>=4;
    if(rx==0) return(0);
    if(rx==0x0FFF) return(0);
    printf("<%u> Error: invalid signed_immed5\n",line);
    return(1);
}
//-------------------------------------------------------------------
unsigned int check_simmed6 ( unsigned int rx )
{
    rx&=0xFFFF;
    rx>>=5;
    if(rx==0) return(0);
    if(rx==0x07FF) return(0);
    printf("<%u> Error: invalid signed_immed6\n",line);
    return(1);
}
//-------------------------------------------------------------------
unsigned int check_offset6 ( unsigned int rx )
{
    rx&=0xFFFF;
    rx>>=5;
    if(rx==0) return(0);
    if(rx==0x07FF) return(0);
    return(1);
}
//-------------------------------------------------------------------
unsigned int check_offset9 ( unsigned int rx )
{
    rx&=0xFFFF;
    rx>>=8;
    if(rx==0) return(0);
    if(rx==0x00FF) return(0);
    return(1);
}
//-------------------------------------------------------------------
unsigned int check_offset11 ( unsigned int rx )
{
    rx&=0xFFFF;
    rx>>=10;
    if(rx==0) return(0);
    if(rx==0x003F) return(0);
    return(1);
}
//-------------------------------------------------------------------
unsigned int parse_immed ( unsigned int ra )
{
    unsigned int rb;

    if((newline[ra]==0x30)&&((newline[ra+1]=='x')||(newline[ra+1]=='X')))
    {
        ra+=2;
        rb=0;
        for(;newline[ra];ra++)
        {
            if(newline[ra]==0x20) break;
            if(!hexchar[newline[ra]])
            {
                printf("<%u> Error: invalid number\n",line);
                return(0);
            }
            cstring[rb++]=newline[ra];
        }
        cstring[rb]=0;
        if(rb==0)
        {
            printf("<%u> Error: invalid number\n",line);
            return(0);
        }
        rx=strtoul(cstring,NULL,16);
    }
    else
    {
        rb=0;
        for(;newline[ra];ra++)
        {
            if(newline[ra]==0x20) break;
            if(!numchar[newline[ra]])
            {
                printf("<%u> Error: invalid number\n",line);
                return(0);
            }
            cstring[rb++]=newline[ra];
        }
        cstring[rb]=0;
        if(rb==0)
        {
            printf("<%u> Error: invalid number\n",line);
            return(0);
        }
        rx=strtoul(cstring,NULL,10);
    }
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_reg ( unsigned int ra )
{
    unsigned int rb;

    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    rb=0;
    for(;newline[ra];ra++)
    {
        if(newline[ra]==',') break;
        if(newline[ra]==0x20) break;
        cstring[rb++]=newline[ra];
    }
    cstring[rb]=0;
    if(get_reg_number(cstring,&rx))
    {
        printf("<%u> Error: not a register\n",line);
        return(0);
    }
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_comma ( unsigned int ra )
{
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    if(newline[ra]!=',')
    {
        printf("<%u> Error: syntax error\n",line);
        return(0);
    }
    ra++;
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_pound ( unsigned int ra )
{
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    if(newline[ra]!='#')
    {
        printf("<%u> Error: syntax error\n",line);
        return(0);
    }
    ra++;
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_three_regs ( unsigned int ra )
{
    ra=parse_reg(ra); if(ra==0) return(0);
    rd=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rs=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rt=rx;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_two_regs ( unsigned int ra )
{
    ra=parse_reg(ra); if(ra==0) return(0);
    rd=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rs=rx;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_three_shift ( unsigned int ra )
{
    ra=parse_reg(ra); if(ra==0) return(0);
    rd=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rt=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rs=rx;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_branch_label ( unsigned int ra )
{
    unsigned int rb;
    unsigned int rc;

    is_const=0;
    is_label=0;
    for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
    if(numchar[newline[ra]])
    {
        ra=parse_immed(ra); if(ra==0) return(0);
        is_const=1;
    }
    else
    {
        //assume label, find space or eol.
        for(rb=ra;newline[rb];rb++)
        {
            if(newline[rb]==0x20) break; //no spaces in labels
        }
        //got a label
        rc=rb-ra;
        if(rc==0)
        {
            printf("<%u> Error: Invalid label\n",line);
            return(0);
        }
        rc--;
        if(rc>=LABLEN)
        {
            printf("<%u> Error: Label too long\n",line);
            return(0);
        }
        for(rb=0;rb<=rc;rb++)
        {
            lab_struct[nlabs].name[rb]=newline[ra++];
        }
        lab_struct[nlabs].name[rb]=0;
        lab_struct[nlabs].addr=curradd;
        lab_struct[nlabs].line=line;
        lab_struct[nlabs].type=1;
        nlabs++;
        rx=0;
        is_label=1;
    }
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_two_label ( unsigned int ra )
{
    ra=parse_reg(ra); if(ra==0) return(0);
    rt=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_reg(ra); if(ra==0) return(0);
    rs=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_branch_label(ra); if(ra==0) return(0);
    rd=rx;
    return(ra);
}
//-------------------------------------------------------------------
unsigned int parse_one_label ( unsigned int ra )
{
    ra=parse_reg(ra); if(ra==0) return(0);
    rs=rx;
    ra=parse_comma(ra); if(ra==0) return(0);
    ra=parse_branch_label(ra); if(ra==0) return(0);
    rd=rx;
    return(ra);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
int assemble ( void )
{
    unsigned int ra;
    unsigned int rb;
    unsigned int rc;


    curradd=0;
    nlabs=0;
    memset(mem,0x00,sizeof(mem));
    memset(mark,0x00,sizeof(mark));

    line=0;
    while(fgets(newline,sizeof(newline)-1,fpin))
    {
        line++;
        //tabs to spaces and other things
        for(ra=0;newline[ra];ra++)
        {
            if(newline[ra]<0x20)  newline[ra]=0x20;
            if(newline[ra]>=0x7F) newline[ra]=0;
        }

        //various ways to comment lines
        for(ra=0;newline[ra];ra++)
        {
            if(newline[ra]==';') newline[ra]=0;
            if(newline[ra]=='@') newline[ra]=0;
            if((newline[ra]=='/')&&(newline[ra+1]=='/')) newline[ra]=0;
            if(newline[ra]==0) break;
        }

        //skip spaces
        for(ra=0;newline[ra];ra++) if(newline[ra]!=0x20) break;
        if(newline[ra]==0) continue;

        //look for a label?
        for(rb=ra;newline[rb];rb++)
        {
            if(newline[rb]==0x20) break; //no spaces in labels
            if(newline[rb]==':') break;
        }
        if(newline[rb]==':')
        {
            //got a label
            rc=rb-ra;
            if(rc==0)
            {
                printf("<%u> Error: Invalid label\n",line);
                return(1);
            }
            rc--;
            if(rc>=LABLEN)
            {
                printf("<%u> Error: Label too long\n",line);
                return(1);
            }
            for(rb=0;rb<=rc;rb++)
            {
                lab_struct[nlabs].name[rb]=newline[ra++];
            }
            lab_struct[nlabs].name[rb]=0;
            lab_struct[nlabs].addr=0x3000+curradd;
            lab_struct[nlabs].line=line;
            lab_struct[nlabs].type=0;
            ra++;
            for(lab=0;lab<nlabs;lab++)
            {
                if(lab_struct[lab].type) continue;
                if(strcmp(lab_struct[lab].name,lab_struct[nlabs].name)==0) break;
            }
            if(lab<nlabs)
            {
                printf("<%u> Error: label [%s] already defined on line %u\n",line,lab_struct[lab].name,lab_struct[lab].line);
                return(1);
            }
            nlabs++;
            //skip spaces
            for(;newline[ra];ra++) if(newline[ra]!=0x20) break;
            if(newline[ra]==0) continue;
        }
// .word -----------------------------------------------------------
        if(strncmp(&newline[ra],".word ",6)==0)
        {
            ra+=6;
            ra=parse_immed(ra); if(ra==0) return(1);
            mem[curradd]=rx;
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// add -----------------------------------------------------------
        if(strncmp(&newline[ra],"add ",4)==0)
        {
            ra+=4;
            //add rd,rs,rx
            //add rd,rs,#imm
            ra=parse_two_regs(ra); if(ra==0) return(1);
            ra=parse_comma(ra); if(ra==0) return(1);
            if(newline[ra]=='#')
            {
                ra=parse_immed(ra+1); if(ra==0) return(1);
                if(check_simmed5(rx)) return(1);
                mem[curradd]=0x1020|(rd<<9)|(rs<<6)|(rx&0x1F);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                ra=parse_reg(ra); if(ra==0) return(1);
                mem[curradd]=0x1000|(rd<<9)|(rs<<6)|rx;
                mark[curradd]|=0x80000000;
                curradd++;
            }
            if(rest_of_line(ra)) return(1);
            continue;
        }
// and -----------------------------------------------------------
        if(strncmp(&newline[ra],"and ",4)==0)
        {
            ra+=4;
            //and rd,rs,rx
            //and rd,rs,#imm
            ra=parse_two_regs(ra); if(ra==0) return(1);
            ra=parse_comma(ra); if(ra==0) return(1);
            if(newline[ra]=='#')
            {
                ra=parse_immed(ra+1); if(ra==0) return(1);
                if(check_simmed5(rx)) return(1);
                mem[curradd]=0x5020|(rd<<9)|(rs<<6)|(rx&0x1F);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                ra=parse_reg(ra); if(ra==0) return(1);
                mem[curradd]=0x5000|(rd<<9)|(rs<<6)|rx;
                mark[curradd]|=0x80000000;
                curradd++;
            }
            if(rest_of_line(ra)) return(1);
            continue;
        }
// jmp -----------------------------------------------------------
        if(strncmp(&newline[ra],"jmp ",4)==0)
        {
            ra+=4;
            //jmp rs
            ra=parse_reg(ra); if(ra==0) return(1);
            mem[curradd]=0xC000|(rx<<6);
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// jsrr -----------------------------------------------------------
        if(strncmp(&newline[ra],"jsrr ",5)==0)
        {
            ra+=5;
            //jsrr rs
            ra=parse_reg(ra); if(ra==0) return(1);
            mem[curradd]=0x4000|(rx<<6);
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// not -----------------------------------------------------------
        if(strncmp(&newline[ra],"not ",4)==0)
        {
            ra+=4;
            //not rd,rs
            ra=parse_two_regs(ra); if(ra==0) return(1);
            mem[curradd]=0x9000|(rd<<9)|(rs<<6);
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// ret -----------------------------------------------------------
        if(strncmp(&newline[ra],"ret",3)==0)
        {
            ra+=3;
            //ret
            mem[curradd]=0xC1C0;
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// rti -----------------------------------------------------------
        if(strncmp(&newline[ra],"rti",3)==0)
        {
            ra+=3;
            //rti
            mem[curradd]=0x8000;
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// brn -----------------------------------------------------------
        if(strncmp(&newline[ra],"brn ",4)==0)
        {
            ra+=4;
            //brn offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0800|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0800;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brz -----------------------------------------------------------
        if(strncmp(&newline[ra],"brz ",4)==0)
        {
            ra+=4;
            //brz offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0400|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0400;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brp -----------------------------------------------------------
        if(strncmp(&newline[ra],"brp ",4)==0)
        {
            ra+=4;
            //brp offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0200|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0200;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brnz -----------------------------------------------------------
        if(strncmp(&newline[ra],"brnz ",5)==0)
        {
            ra+=5;
            //brnzp offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0C00|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0C00;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brnp -----------------------------------------------------------
        if(strncmp(&newline[ra],"brnp ",5)==0)
        {
            ra+=5;
            //brnp offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0A00|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0A00;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brzp -----------------------------------------------------------
        if(strncmp(&newline[ra],"brzp ",5)==0)
        {
            ra+=5;
            //brzp offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0600|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0600;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// brnzp -----------------------------------------------------------
        if(strncmp(&newline[ra],"brnzp ",6)==0)
        {
            ra+=6;
            //brnzp offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: branch too far\n",line);
                    return(1);
                }
                mem[curradd]=0x0E00|(rx&0x1FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x0E00;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// jsr -----------------------------------------------------------
        if(strncmp(&newline[ra],"jsr ",4)==0)
        {
            ra+=4;
            //jsr offset/label
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset11(rx))
                {
                    printf("<%u> Error: jsr too far\n",line);
                    return(1);
                }
                mem[curradd]=0x4800|(rx&0x07FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x4800;
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// ld -----------------------------------------------------------
        if(strncmp(&newline[ra],"ld ",3)==0)
        {
            ra+=3;
            //ld rd,offset/label
            ra=parse_reg(ra); if(ra==0) return(1);
            rd=rx;
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: ld too far\n",line);
                    return(1);
                }
                mem[curradd]=0x2000|(rd<<9)|(rx&0x01FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x2000|(rd<<9);
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// ldi -----------------------------------------------------------
        if(strncmp(&newline[ra],"ldi ",4)==0)
        {
            ra+=4;
            //ldi rd,offset/label
            ra=parse_reg(ra); if(ra==0) return(1);
            rd=rx;
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: ldi too far\n",line);
                    return(1);
                }
                mem[curradd]=0xA000|(rd<<9)|(rx&0x01FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0xA000|(rd<<9);
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// ldr -----------------------------------------------------------
        if(strncmp(&newline[ra],"ldr ",4)==0)
        {
            ra+=4;
            //ldr rd,rs,#offset
            ra=parse_two_regs(ra); if(ra==0) return(1);
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_pound(ra); if(ra==0) return(1);
            ra=parse_immed(ra); if(ra==0) return(1);
            if(rest_of_line(ra)) return(1);
            if(check_simmed6(rx)) return(1);
            mem[curradd]=0x6000|(rd<<9)|(rs<<6)|(rx&0x003F);
            mark[curradd]|=0x80000000;
            curradd++;
            continue;
        }
// lea -----------------------------------------------------------
        if(strncmp(&newline[ra],"lea ",4)==0)
        {
            ra+=4;
            //ldi rd,offset/label
            ra=parse_reg(ra); if(ra==0) return(1);
            rd=rx;
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: lea too far\n",line);
                    return(1);
                }
                mem[curradd]=0xE000|(rd<<9)|(rx&0x01FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0xE000|(rd<<9);
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// st -----------------------------------------------------------
        if(strncmp(&newline[ra],"st ",3)==0)
        {
            ra+=3;
            //st rd,offset/label
            ra=parse_reg(ra); if(ra==0) return(1);
            rd=rx;
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: st too far\n",line);
                    return(1);
                }
                mem[curradd]=0x3000|(rd<<9)|(rx&0x01FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0x3000|(rd<<9);
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// sti -----------------------------------------------------------
        if(strncmp(&newline[ra],"sti ",4)==0)
        {
            ra+=4;
            //sti rd,offset/label
            ra=parse_reg(ra); if(ra==0) return(1);
            rd=rx;
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_branch_label(ra); if(ra==0) return(0);
            if(rest_of_line(ra)) return(1);
            if(is_const)
            {
                if(check_offset9(rx))
                {
                    printf("<%u> Error: sti too far\n",line);
                    return(1);
                }
                mem[curradd]=0xB000|(rd<<9)|(rx&0x01FF);
                mark[curradd]|=0x80000000;
                curradd++;
            }
            else
            {
                mem[curradd]=0xB000|(rd<<9);
                mark[curradd]|=0x80000002;
                curradd++;
            }
            continue;
        }
// str -----------------------------------------------------------
        if(strncmp(&newline[ra],"str ",4)==0)
        {
            ra+=4;
            //str rd,rs,#offset
            ra=parse_two_regs(ra); if(ra==0) return(1);
            ra=parse_comma(ra); if(ra==0) return(1);
            ra=parse_pound(ra); if(ra==0) return(1);
            ra=parse_immed(ra); if(ra==0) return(1);
            if(rest_of_line(ra)) return(1);
            if(check_simmed6(rx)) return(1);
            mem[curradd]=0x7000|(rd<<9)|(rs<<6)|(rx&0x003F);
            mark[curradd]|=0x80000000;
            curradd++;
            continue;
        }
// trap -----------------------------------------------------------
        if(strncmp(&newline[ra],"trap ",5)==0)
        {
            ra+=5;
            //trap imm
            ra=parse_immed(ra); if(ra==0) return(1);
            if((rx&0xFF)!=rx)
            {
                printf("<%u> Error: invalid trap vector\n",line);
                return(1);
            }
            mem[curradd]=0xF000|(rx&0xFF);
            mark[curradd]|=0x80000000;
            curradd++;
            if(rest_of_line(ra)) return(1);
            continue;
        }
// -----------------------------------------------------------
        printf("<%u> Error: syntax error\n",line);
        return(1);
    }
    return(0);
}
//-------------------------------------------------------------------
int main ( int argc, char *argv[] )
{
    int ret;
    unsigned int ra;
    unsigned int rb;
    unsigned int inst;
    unsigned int inst2;

    if(argc!=2)
    {
        printf("lc3asm filename\n");
        return(1);
    }

    fpin=fopen(argv[1],"rt");
    if(fpin==NULL)
    {
        printf("Error opening file [%s]\n",argv[1]);
        return(1);
    }
    ret=assemble();
    fclose(fpin);

    if(ret) return(1);
    for(ra=0;ra<nlabs;ra++)
    {
        printf("label%04u: [0x%08X] [%s] %u\n",ra,lab_struct[ra].addr,lab_struct[ra].name,lab_struct[ra].type);

        if(lab_struct[ra].type)
        {
            for(rb=0;rb<nlabs;rb++)
            {
                if(lab_struct[rb].type) continue;
                if(strcmp(lab_struct[ra].name,lab_struct[rb].name)==0)
                {
                    rx=lab_struct[rb].addr;
                    inst=mem[lab_struct[ra].addr];

                    if((inst&0xF000)==0x0000)
                    {
                        //br[n][z][p]
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: branch too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF000)==0x2000)
                    {
                        //ld
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: ld too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF000)==0xA000)
                    {
                        //ldi
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: ldi too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF800)==0x4800)
                    {
                        //jsr
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset11(rd))
                        {
                            printf("<%u> Error: jsr too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x07FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF000)==0xE000)
                    {
                        //lea
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: lea too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF000)==0x3000)
                    {
                        //st
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: st too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }
                    if((inst&0xF000)==0xB000)
                    {
                        //sti
                        rs=0x3000+(lab_struct[ra].addr);
                        rd=rx-(rs+1);
                        if(check_offset9(rd))
                        {
                            printf("<%u> Error: sti too far\n",lab_struct[ra].line,inst);
                            return(1);
                        }
                        inst|=rd&0x01FF;
                        lab_struct[ra].type++;
                    }

                    if(lab_struct[ra].type==1)
                    {
                        printf("<%u> Error: internal error, unknown instruction 0x%08X\n",lab_struct[ra].line,inst);
                        return(1);
                    }
                    mem[lab_struct[ra].addr]=inst;
                    break;
                }
            }
            if(rb<nlabs) ; else
            {
                printf("<%u> Error: unresolved label\n",lab_struct[ra].line);
                return(1);
            }
        }
    }
    sprintf(newline,"%s.hex",argv[1]);
    fpout=fopen(newline,"wt");
    if(fpout==NULL)
    {
        printf("Error creating file [%s]\n",newline);
        return(1);
    }

    for(ra=0;ra<=ADDMASK;ra++)
    {
        curradd=0x3000+ra;
        if(mark[ra]&0x80000000)
        {
            printf("0x%04X: 0x%04X\n",curradd,mem[ra]);

            rb=0x02;
            rb+=(curradd>>8)&0xFF;
            rb+=(curradd>>0)&0xFF;
            rb+=0x00;
            rb+=(mem[ra]>> 8)&0xFF;
            rb+=(mem[ra]>> 0)&0xFF;
            rb=(-rb)&0xFF;
            fprintf(fpout,":%02X%04X%02X%04X%02X\n",0x02,curradd&0xFFFF,0x00,mem[ra],rb);
        }
    }
    fclose(fpout);
    return(0);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------


