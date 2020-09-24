#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "disassembler.h"

unsigned long long offset = 0;
unsigned long long* offsets = NULL;
int id = 0;


char* reg_name(int reg) {
    char* res = malloc(sizeof(char)*4);
    switch (reg) {
        case(0): res = "eax"; break;
        case(1): res = "ecx"; break;
        case(2): res = "edx"; break;
        case(3): res = "ebx"; break;
        case(4): res = "esp"; break;
        case(5): res = "ebp"; break;
        case(6): res = "esi"; break;
        case(7): res = "edi"; break;
        case(8): res = ""; break;
    }
    return res;
}

drr* decode_MODRM(unsigned char modrm) {
    drr* res = malloc(sizeof(drr)*1);
    unsigned char temp = modrm;
    unsigned int mode = temp & MODE;
    mode = mode >> 6;
    unsigned int reg = temp & REG;
    reg = reg >> 3;
    unsigned int rm = temp & RM;

    switch (mode) {
        case (0):
            if (rm==5) {
                res->disp = 4;
                rm = 8;
                res->mode = 4;  //special
            }
            else 
                res->disp = 0;
            break;
        case(1):
            res->mode = 1;
            res->disp = 1;
            break;
        case(2):
            res->mode = 2;
            res->disp = 4;
            break;
        case(3):
            res->mode = 3;
            res->disp = 0;
            break;
    }

    res->reg = reg_name(reg);
    res->rm = reg_name(rm);
    return res;
}

void print_DISP(drr* res, unsigned char* opcode) {
    int disp = res->disp;
    if (disp==1)
        printf("%02X", *(opcode+2));
    else if (disp==4)
        printf("%02X%02X%02X%02X", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
}

unsigned int disasm(unsigned char* buffer, unsigned int pc) {
    unsigned int operand = 0;
    unsigned char* opcode = &buffer[pc];
    drr* res;
    if (ADDRESS+pc==offsets[0] && ADDRESS+pc!=0) {
        printf("offset_%08Xh:\n", offsets[0]);
        remove_offset();
    }
    printf("0x%08X:\t", ADDRESS + pc);

    switch (*opcode) {
        case 0x81:    
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE + IMM32;
            //add r/m32, imm32 0x81 /0 id
            if (strcmp(res->reg, "eax")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("add [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("add [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("add %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("add [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("add [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            } 
            //and r/m32, imm32 0x81 /4 id
            else if (strcmp(res->reg, "esp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("and [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("and [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("and %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("and [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("and [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            //cmp r/m32, imm32 0x81 /7 id
            else if (strcmp(res->reg, "edi")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("cmp [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("cmp [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("cmp %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("cmp [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("cmp [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            //or r/m32, imm32 0x81 /1 id
            else if (strcmp(res->reg, "ecx")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("or [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("or [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("or %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("or [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("or [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            //sbb r/m32, imm32 0x81 /3 id
            else if (strcmp(res->reg, "ebx")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("sbb [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("sbb [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("sbb %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("sbb [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("sbb [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            //sub r/m32, imm32 0x81 /5 id
            else if (strcmp(res->reg, "ebp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("sub [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("sub [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("sub %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("sub [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("sub [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            //xor r/m32, imm32 0x81 /6 id
            else if (strcmp(res->reg, "esi")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("xor [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("xor [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("xor %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("xor [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("xor [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            }
            else {
                printf("db 0x%02X", *opcode);
                operand = 0;
            }
            break;

        case 0xFF:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //call r/m32 0xFF /2
            if (strcmp(res->reg, "edx")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("call [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("call [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("call %s", res->rm);
                else if (res->mode==4)
                    printf("call [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("call [%s]", res->rm);
            } 
            //dec r/m32 0xFF /1       
            else if (strcmp(res->reg, "ecx")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("dec [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("dec [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("dec %s", res->rm);
                else if (res->mode==4)
                    printf("dec [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("dec [%s]", res->rm);
            } 
            //inc r/m32 0xFF /0
            else if (strcmp(res->reg, "eax")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("inc [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("inc [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("inc %s", res->rm);
                else if (res->mode==4)
                    printf("inc [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("inc [%s]", res->rm);
            } 
            //jmp r/m32 0xFF /4
            else if (strcmp(res->reg, "esp")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("jmp [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("jmp [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("jmp %s", res->rm);
                else if (res->mode==4)
                    printf("jmp [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("jmp [%s]", res->rm);
            } 
            //push r/m32 0xFF /6
            else if (strcmp(res->reg, "esi")==0) {   
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("push [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("push [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("push %s", res->rm);
                else if (res->mode==4)
                    printf("push [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("push [%s]", res->rm);
            } 
            else {
                printf("db 0x%02X", *opcode);
                operand = 0;
            }
                        
            break;

        case 0x0F:
            
            //clflush m8 0x0F 0xAE /7, no 11
            if (*(opcode+1)==0xAE && strcmp(res->reg, "edi")==0) {
                res = decode_MODRM(*(opcode+2));
                printf("%02X%02X%02X", *opcode, *(opcode+1), *(opcode+2));
                if (res->disp==1)
                    printf("%02X", *(opcode+3));
                else if (res->disp==4)
                    printf("%02X%02X%02X%02X", *(opcode+3),*(opcode+4),*(opcode+5),*(opcode+6));
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("clflush [%s+0x%02X]", res->rm, *(opcode+3));
                else if (res->mode==2)
                    printf("clflush [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==3) {
                    printf("db 0x%02X", *opcode);
                    operand = 0;
                    break;
                }
                else if (res->mode==4)
                    printf("clflush [0x%02X%02X%02X%02X]", *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else printf("clflush [%s]", res->rm);
                operand = res->disp + MODRMBYTE + 1;
            }
            //imul r32, r/m32 0x0F 0xAF /r
            else if (*(opcode+1)==0xAF) {
                res = decode_MODRM(*(opcode+2));
                printf("%02X%02X%02X", *opcode, *(opcode+1), *(opcode+2));
                if (res->disp==1)
                    printf("%02X", *(opcode+3));
                else if (res->disp==4)
                    printf("%02X%02X%02X%02X", *(opcode+3),*(opcode+4),*(opcode+5),*(opcode+6));
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("imul %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+3));
                else if (res->mode==2)
                    printf("imul %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==3)
                    printf("imul %s, %s", res->reg, res->rm);
                else if (res->mode==4)
                    printf("imul %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else printf("imul %s, [%s]", res->reg, res->rm);
                operand = res->disp + MODRMBYTE + 1;
            }
            //jz rel32 0x0F 0x84 cd
            else if (*(opcode+1)==0x84) {
                printf("%02X%02X", *opcode, *(opcode+1));
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
                unsigned long long offset = *(opcode+5)*65536+ *(opcode+4)*4096+ *(opcode+3)*256+ *(opcode+2);
                operand = IMM32;
                offset += ADDRESS+pc+operand+OPBYTE+1;
                printf("jz offset_%08Xh", offset);
                insert_offset(offset);
                operand += 1;
            }
            //jnz rel32 0x0F 0x85 cd
            else if (*(opcode+1)==0x85) {
                printf("%02X%02X", *opcode, *(opcode+1));
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
                unsigned long long offset = *(opcode+5)*65536+ *(opcode+4)*4096+ *(opcode+3)*256+ *(opcode+2);
                operand = IMM32;
                offset += ADDRESS+pc+operand+OPBYTE+1;
                printf("jnz offset_%08Xh", offset);
                insert_offset(offset);
                operand += 1;
            }
            else {
                printf("db 0x%02X", *opcode);
                operand = 0;
            }
            break;

        case 0xF7:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //idiv r/m32 0xF7 /7
            if (strcmp(res->reg, "edi")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("idiv [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("idiv [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("idiv %s", res->rm);
                else if (res->mode==4)
                    printf("idiv [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("idiv [%s]", res->rm);
            }
            //imul r/m32 0xF7 /5
            else if (strcmp(res->reg, "ebp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("imul [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("imul [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("imul %s", res->rm);
                else if (res->mode==4)
                    printf("imul [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("imul [%s]", res->rm);
            }
            //mul r/m32 0xF7 /4
            else if (strcmp(res->reg, "esp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("mul [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("mul [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("mul %s", res->rm);
                else if (res->mode==4)
                    printf("mul [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("mul [%s]", res->rm);
            }
            //neg r/m32 0xF7 /3
            else if (strcmp(res->reg, "ebx")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("neg [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("neg [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("neg %s", res->rm);
                else if (res->mode==4)
                    printf("neg [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("neg [%s]", res->rm);
            }
            //not r/m32 0xF7 /2
            else if (strcmp(res->reg, "edx")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("not [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("not [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("not %s", res->rm);
                else if (res->mode==4)
                    printf("not [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("not [%s]", res->rm);
            }
            //test r/m32, imm32 0xF7 /0 id
            else if (strcmp(res->reg, "eax")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                if (res->mode==1) 
                    printf("test [%s+0x%02X], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    printf("test [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    printf("test %s, 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    printf("test [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else printf("test [%s], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                operand += IMM32;
            }
            else {
                printf("db 0x%02X", *opcode);
                operand = 0;
            }
            break;

        case 0xD1:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //sal r/m32, 1 0xD1 /4
            if (strcmp(res->reg, "esp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("sal [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("sal [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("sal %s, 1", res->rm);
                else if (res->mode==4)
                    printf("sal [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("sal [%s], 1", res->rm);
            }
            //sar r/m32, 1 0xD1 /7
            else if (strcmp(res->reg, "edi")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("sar [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("sar [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("sar %s, 1", res->rm);
                else if (res->mode==4)
                    printf("sar [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("sar [%s], 1", res->rm);
            }
            //shr r/m32, 1 0xD1 /5
            else if (strcmp(res->reg, "ebp")==0) {
                printf("%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                printf("\t\t\t");
                if (res->mode==1) 
                    printf("shr [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    printf("shr [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    printf("shr %s, 1", res->rm);
                else if (res->mode==4)
                    printf("shr [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else printf("shr [%s], 1", res->rm);
            }
            else {
                printf("db 0x%02X", *opcode);
                operand = 0;
            }
            break;

        case 0x05:    //add eax, imm32 0x05 id, no MODRM
            printf("%02X", *opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("add eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x01:    //add r/m32, r32 0x01 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("add [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("add [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("add %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("add [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("add [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x03:    //add r32, r/m32 0x03 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("add %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("add %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("add %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("add %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("add %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x25:    //and eax, imm32 0x25 id, no MODRM
            printf("%02X", *opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("and eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x21:    //and r/m32, r32 0x21 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("and [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("and [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("and %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("and [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("and [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x23:    //and r32, r/m32 0x23 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("and %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("and %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("and %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("and %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("and %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0xE8:    //call rel32 0xE8 cd
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            offset = *(opcode+4)*65536+ *(opcode+3)*4096+ *(opcode+2)*256+ *(opcode+1);
            operand = IMM32;
            offset += ADDRESS+pc+operand+OPBYTE;
            printf("call offset_%08Xh", offset);
            insert_offset(offset);
            break;

        case 0x3D:    //cmp eax, imm32 0x3D id, no MODRM
            printf("%02X", *opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("cmp eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x39:    //cmp r/m32, r32 0x39 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("cmp [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("cmp [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("cmp %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("cmp [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("cmp [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x3B:    //cmp r32, r/m32 0x3B /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("cmp %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("cmp %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("cmp %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("cmp %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("cmp %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x48:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec eax");
            break;

        case 0x49:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec ecx");
            break;
        
        case 0x4A:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec edx");
            break;
        
        case 0x4B:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec ebx");
            break;
        
        case 0x4C:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec esp");
            break;
        
        case 0x4D:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec ebp");
            break;
        
        case 0x4E:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec esi");
            break;
        
        case 0x4F:    //dec r32 0x48 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("dec edi");
            break;
        
        case 0x69:    //imul r32, r/m32, imm32 0x69 /r id
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
            if (res->mode==1) 
                printf("imul %s, [%s+0x%02X], 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
            else if (res->mode==2)
                printf("imul %s, [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else if (res->mode==3)
                printf("imul %s, %s, 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                printf("imul %s, [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X",  
                    res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else printf("imul %s, [%s], 0x%02X%02X%02X%02X", 
                res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            operand = res->disp + MODRMBYTE + IMM32;
            break;

        case 0x40:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc eax");
            break;

        case 0x41:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc ecx");
            break;
        
        case 0x42:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc edx");
            break;
        
        case 0x43:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc ebx");
            break;
        
        case 0x44:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc esp");
            break;
        
        case 0x45:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc ebp");
            break;
        
        case 0x46:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc esi");
            break;
        
        case 0x47:    //inc r32 0x40 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("inc edi");
            break;

        case 0xEB:    //jmp rel8 0xEB cb
            printf("%02X%02X\t\t\t", *opcode, *(opcode+1));
            offset = *(opcode+1);
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            printf("jmp offset_%08Xh", offset);
            insert_offset(offset);
            break;

        case 0xE9:    //jmp rel32 0xE9 cd
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            offset = *(opcode+4)*65536+ *(opcode+3)*4096+ *(opcode+2)*256+ *(opcode+1);
            operand = IMM32;
            offset += ADDRESS+pc+operand+OPBYTE;
            printf("jmp offset_%08Xh", offset);
            insert_offset(offset);
            break;

        case 0x74:    //jz rel8 0x74 cb
            printf("%02X%02X\t\t\t", *opcode, *(opcode+1));
            offset = *(opcode+1);
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            printf("jz offset_%08Xh", offset);
            insert_offset(offset);
            break;

        case 0x75:    //jnz 0x75 cb
            printf("%02X%02X\t\t\t", *opcode, *(opcode+1));
            offset = *(opcode+1);
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            printf("jnz offset_%08Xh", offset);
            insert_offset(offset);
            break;

        case 0x8D:    //lea r32, m 0x8D /r, no 11
            res = decode_MODRM(*(opcode+1));
            if (res->mode==3) {
                printf("db 0x%02X", *opcode);
                operand = 0;
                break;
            }
            printf("%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("lea %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("lea %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                printf("lea %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("lea %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0xB8:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xB9:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov ecx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBA:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov edx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBB:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov ebx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBC:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov esp, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBD:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov ebp, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBE:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov esi, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xBF:    //mov r32, imm32 0xB8+rd id
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("mov edi, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xC7:    //mov r/m32, imm32 0xC7 /0 id
            res = decode_MODRM(*(opcode+1));
            if (strcmp(res->reg, "eax")!=0) {
                printf("db 0x%02X", *opcode);
                operand = 0;
                break;
            }
            printf("%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
            if (res->mode==1) 
                printf("mov [%s+0x%02X], 0x%02X%02X%02X%02X", res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
            else if (res->mode==2)
                printf("mov [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                    res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else if (res->mode==3)
                printf("mov %s, 0x%02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                printf("mov [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else printf("mov [%s], 0x%02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            operand = res->disp + MODRMBYTE + IMM32;
            break;

        case 0x89:    //mov r/m32, r32 0x89 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("mov [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("mov [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("mov %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("mov [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("mov [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x8B:    //mov r32, r/m32 0x8B /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("mov %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("mov %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("mov %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("mov %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("mov %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0xA5: //TODO   //movsd 0xA5, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("movsd");
            break;

        case 0x90:    //nop 0x90, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("nop");
            break;
            
        case 0x0D:    //or eax, imm32 0x0D id, no MODRM
            printf("%02X", *opcode);
            printf("%02X%02X%02X%02X\t\t\t", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("or eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x09:    //or r/m32, r32 0x09 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("or [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("or [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("or %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("or [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("or [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x0B:    //or r32, r/m32 0x0B /r 
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("or %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("or %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("or %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("or %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("or %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0xE7:    //out imm8, eax 0xE7 ib, no MODRM
            printf("%02X%02X\t\t\t", *opcode, *(opcode+1));
            printf("out 0x%02X, eax", *(opcode+1));
            operand = IMM8;
            break;

        case 0x8F:    //pop r/m32 0x8F /0
            res = decode_MODRM(*(opcode+1));
            if (strcmp(res->reg, "eax")!=0) {
                printf("db 0x%02X", *opcode);
                operand = 0;
                break;
            }
            printf("%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("pop [%s+0x%02X]", res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("pop [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("pop %s", res->rm);
            else if (res->mode==4)
                printf("pop [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("pop [%s]", res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x58:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop eax");
            break;

        case 0x59:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop ecx");
            break;
        
        case 0x5A:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop edx");
            break;
        
        case 0x5B:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop ebx");
            break;
        
        case 0x5C:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop esp");
            break;
        
        case 0x5D:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop ebp");
            break;
        
        case 0x5E:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop esi");
            break;
        
        case 0x5F:    //pop r32 0x58 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("pop edi");
            break;

        case 0x50:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push eax");
            break;

        case 0x51:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push ecx");
            break;

        case 0x52:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push edx");
            break;

        case 0x53:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push ebx");
            break;

        case 0x54:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push esp");
            break;

        case 0x55:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push ebp");
            break;

        case 0x56:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push esi");
            break;

        case 0x57:    //push r32 0x50 + rd, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("push edi");
            break;

        case 0x68:    //push imm32 0x68 id, no MODRM
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("push 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0xF2:    //repne cmpsd 0xF2 0xA7
            if (*(opcode+1)!=0xA7) {
                printf("db 0x%02X", *opcode);
                break;
            }
            printf("%02X%02X\t\t\t", *opcode, *(opcode+1));
            printf("repne cmpsd");
            break;

        case 0xCB:    //retf 0xCB, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("retf");
            break;

        case 0xCA:    //retf imm16 0xCA iw, no MODRM
            printf("%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2));
            printf("retf 0x%02X%02X", *(opcode+2),*(opcode+1));
            operand = IMM16;
            break;

        case 0xC3:    //retn 0xC3, no MODRM
            printf("%02X\t\t\t", *opcode);
            printf("retn");
            break;

        case 0xC2:    //retn imm16 0xC2 iw, no MODRM
            printf("%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2));
            printf("retn 0x%02X%02X", *(opcode+2),*(opcode+1));
            operand = IMM16;
            break;

        case 0x1D:    //sbb eax, imm32 0x1D id, no MODRM
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("sbb eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x19:    //sbb r/m32, r32 0x19 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("sbb [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("sbb [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("sbb %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("sbb [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("sbb [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x1B:    //sbb r32, r/m32 0x1B /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("sbb %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("sbb %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("sbb %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("sbb %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("sbb %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x2D:    //sub eax, imm32 0x2D id, no MODRM
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("sub eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x29:    //sub r/m32, r32 0x29 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("sub [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("sub [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("sub %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("sub [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("sub [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x2B:    //sub r32, r/m32 0x2B /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("sub %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("sub %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("sub %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("sub %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("sub %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;

        case 0xA9:    //test eax, imm32 0xA9 id, no MODRM
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("test eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x85:    //test r/m32, r32 0x85 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("test [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("test [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("test %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("test [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("test [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x35:    //xor eax, imm32 0x35 id, no MODRM
            printf("%02X%02X%02X%02X%02X\t\t\t", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            printf("xor eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            break;

        case 0x31:    //xor r/m32, r32 0x31 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("xor [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                printf("xor [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                printf("xor %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                printf("xor [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else printf("xor [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            break;

        case 0x33:    //xor r32, r/m32 0x33 /r
            printf("%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            printf("\t\t\t");
            if (res->mode==1) 
                printf("xor %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                printf("xor %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                printf("xor %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                printf("xor %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else printf("xor %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            break;
        
        default:
            printf("db 0x%02X", *opcode);
            break;
    }

    printf("\n");
    return OPBYTE+operand;
}


int main(int argc, char const *argv[]) {
	if (argc < 3 || strcmp(argv[1], "-i")) {
		printf("Please specify the input file name using the '-i' option.\n");
		exit(1);
	}
	FILE *fp = fopen(argv[2], "rb");
    if (!fp) {
        printf("File %s does not exist.\n", argv[2]);
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    unsigned char* buffer = malloc(fsize);
    fread(buffer, fsize, 1, fp);
    fclose(fp);
    
    init_offset();
    unsigned int pc = 0;
    while (pc < fsize) 
        pc += disasm(buffer, pc);
    
	return 0;
}

void init_offset() {
    offsets = calloc(300, sizeof(unsigned long long));
}

int compare(const void *a, const void *b) {
    unsigned long long* x = (unsigned long long*) a; 
    unsigned long long* y = (unsigned long long*) b;
    return *x - *y;
}

void insert_offset(unsigned long long n) {
    offsets[id] = n;
    qsort(offsets, id+1, sizeof(unsigned long long), compare);
    id+=1;
}

void remove_offset() {
    for (int i = 0; i < 300-1; i++) {
        offsets[i] = offsets[i+1];
        if (offsets[i]==0) break;
    }
    id-=1;
}