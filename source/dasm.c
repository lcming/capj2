#include <stdio.h>
#include <stdlib.h>

#define BASE 0x00400000
#define BSTAGE 3
//#define BSTAGE EX
//#define BSTAGE MEM
FILE* fout;
int prev = -1;
int prev2 = -1;
int cnt = 0;

enum OPCODE
{
    arith = 0x0,
    mul = 0x1C,

    addi =0x8,
    addiu=0x9,
    slti =0xA,
    sltiu=0xB,
    andi =0xC,
    ori  =0xD,
    xori =0xE,

    lw = 0x23,
    sw = 0x2B,
    lbu= 0x24,
    lb = 32,
    sb = 0x28,
    beq= 0x4,
    bne= 0x5,
    blez=0x6,
    bgtz=0x7,
    bltz=0x1,
    j   =0x2,
    jal =0x3,

};
enum FUCODE
{
    srl = 0x02,
    lui = 0x0f,
    add = 0x20,
    addu= 0x21,
    sub = 0x22,
    subu= 0x23,
    //mulu= 0x19,
    //Div = 0x1A, 
    divu = 0x1B,
    slt = 0x2A,
    sltu = 0x2B,
    and = 0x24,
    or = 0x25,
    nor = 0x27,
    xor = 0x28,
    sysc= 0x0C,
    jr  = 0x08,
};
enum TYPE
{
    RTYPE,
    ITYPE,
    JTYPE,
};


void exception(char* msg)
{
    printf("fatal: %s\n", msg);
    exit(1);
}
void assert(int result)
{
    if(!result)
    {
        printf("fatal: assertion fail\n");
        exit(1);
    }
}
void hazard(int rs, int rt, int rd)
{
    if(prev >= 0 && (rs == prev || rt == prev) )
    {
        fprintf(fout, "nop #data hazard\n");
        fprintf(fout, "nop #data hazard\n");
    }
    else if(prev2 >= 0 && (rs == prev2 || rt == prev2) )
    {
        fprintf(fout, "nop #data hazard\n");
    }
    prev2 = prev;
    prev = rd;
}

void printM(int ir, char* head)
{
    int fucode = mask(ir, 5, 0);
    int rs, rt, rd;
    rs = mask(ir, 25, 21);
    rt = mask(ir, 20, 16);
    rd = mask(ir, 15, 11);
    hazard(rs, rt, rd);
    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s $%d, $%d, $%d\n", head, rd, rs, rt);
}

void printR(int ir, char* head)
{
    int fucode = mask(ir, 5, 0);
    int rs, rt, rd;
    if(fucode == sysc)
    {
        hazard(-1, -1, -1);
        fprintf(fout, "syscall\n");
        return;
    }
    else if(fucode == jr)
    {

        head = "jr";
        rs = mask(ir, 25, 21);
        hazard(rs, -1, -1);
        if(cnt)
            fprintf(fout, "tag%d:", cnt);
        fprintf(fout, "%s $%d\n", head, rs);
        return;
    }
    else if(fucode == srl)
    {
        head = "srl";
        rs = mask(ir, 20, 16);
        rt = mask(ir, 15, 11);
        int shm = mask(ir, 10, 6);
        hazard(-1, rs, rt);
        if(cnt)
            fprintf(fout, "tag%d:", cnt);
        fprintf(fout, "%s $%d, $%d, %d  #%X\n", head, rt, rs, shm, ir);
        return;
    }

    rs = mask(ir, 25, 21);
    rt = mask(ir, 20, 16);
    rd = mask(ir, 15, 11);
    switch(fucode)
    {
        case add: head  =   "add"; break;
        case addu: head =   "addu"; break;
        case sub: head  =   "sub"; break;
        case subu: head =   "subu"; break;
        //case mulu: head =   "mulu"; break;
        case slt: head  =   "slt"; break;
        case sltu: head =   "sltu";break;
        case and: head  =   "and"; break;
        case or: head   =   "or";  break;
        case nor: head  =   "nor"; break;
        case xor: head  =   "xor"; break;
        case jr:  head  =   "jr" ; break;
        default:
            printf("%X:\n", fucode);
            exception("no match instruction type\n");
    }
    hazard(rs, rt, rd);
    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s $%d, $%d, $%d\n", head, rd, rs, rt);
}

void printLS(int ir, char* head)
{
    int rs = mask(ir, 25, 21);
    int rt = mask(ir, 20, 16);
    int offset = mask_signed(ir, 15, 0);
    hazard(-1, rs, rt);
    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s $%d, %d($%d)\n", head, rt, offset, rs);
}
void printIM(int ir, char* head)
{
    int rs = mask(ir, 25, 21);
    int rt = mask(ir, 20, 16);
    int offset = mask_signed(ir, 15, 0);
    hazard(-1, rs, rt);
    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s, $%d, $%d, %d\n", head, rt, rs, offset);
}
void printLIM(int ir, char* head)
{
    int rt = mask(ir, 20, 16);
    int offset = mask_signed(ir, 15, 0);
    hazard(-1, -1, rt);
    if(cnt)
        fprintf(fout,"tag%d:", cnt);
    fprintf(fout, "%s, $%d, %d\n", head, rt, offset);
}
void printB(int ir, char* head)
{
    int rs = mask(ir, 25, 21);
    int rt = mask(ir, 20, 16);
    int offset = mask_signed(ir, 15, 0);
    hazard(rs, rt, -1);

    int j;
    for(j = 0; j < BSTAGE; j++)
        fprintf(fout, "nop # control hazard\n");

    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s $%d, $%d, tag%d\n", head, rs, rt, cnt+offset+1);
}
void printJ(int ir, char* head)
{
    int target = mask(ir, 25, 0);
    target = ( (target*4 - BASE)>>2 );
    hazard(-1, -1, -1);
    if(cnt)
        fprintf(fout, "tag%d:", cnt);
    fprintf(fout, "%s tag%d\n", head, target);
}

int mask(int ir, int msb, int lsb)
{
    assert(msb >= lsb);
    int wid = msb - lsb + 1;
    unsigned int _mask = (1 << wid) - 1;
    return ( ( ir >> lsb ) & _mask );
}

int mask_signed(int ir, int msb, int lsb)
{
    assert(msb >= lsb);
    int wid = msb - lsb + 1;
    unsigned int _mask = (1 << wid) - 1;
    int ret = ( ( ir >> lsb ) & _mask );
    if( ( ir >> msb ) & 1 )  // get msb
    {
        // neg
        return -( (1 << wid) - ret);
    }
    else
        return ret;
}

int main(int argc, char* argv[]) 
{ 

    FILE* fin = fopen(argv[1], "r");
    fout = fopen(argv[2], "w");

    fprintf(fout, ".data\nqmat:  .word 1, 1, 1, 0\n");
    fprintf(fout, ".text\n.globl main\nmain:");
    
    while( !feof(fin) )
    {
        char* head;
        int ins;
        fscanf(fin, "%x", &ins);
        unsigned int opcode = mask (ins, 31, 26);
        switch(opcode)
        {
            case arith:
                printR(ins, head);
                break;
            case mul:
                head = "mul";
                printM(ins, head);
                break;
            case lw:  
                head = "lw";//lw
                printLS(ins, head);
                break;
            case sw:  
                head = "sw";//sw
                printLS(ins, head);
                break;
            case lb:  
                head = "lb";//lw
                printLS(ins, head);
                break;
            case sb:  
                head = "sb";//sw
                printLS(ins, head);
                break;
            case addi:
                head = "addi";
                printIM(ins, head);
                break;
            case addiu:
                head = "addiu";
                printIM(ins, head);
                break;
            case slti:
                head = "slti";
                printIM(ins, head);
                break;
            case sltiu:
                head = "sltiu";
                printIM(ins, head);
                break;
            case andi:
                head = "andi";
                printIM(ins, head);
                break;
            case ori:
                head = "ori";
                printIM(ins, head);
                break;
            case xori:
                head = "xori";
                printIM(ins, head);
                break;
            case lui:
                head = "lui";
                printLIM(ins, head);
                break;
            case beq:  //beq
                head = "beq";
                printB(ins, head);
                break;
            case bne:  //bne
                head = "bne";
                printB(ins, head);
                break;
            case   j:
                head = "j";
                printJ(ins, head);
                break;
            case jal:
                head = "jal";
                printJ(ins, head);
                break;
            default:
                printf("OP=%X, ", opcode);
                exception("undefined op\n");
        }
        cnt++;
    }
}
