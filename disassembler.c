#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "disassembler.h"

long long offset = 0;
long long* offsets = NULL;
int id = 0;
strbuf* head = NULL;
strbuf* tail = NULL;
char* buf = NULL;
char* tempbuf = NULL;
char* section = NULL;

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
    
    buf = calloc(100, sizeof(char));
    tempbuf = calloc(100, sizeof(char));
    section = calloc(50, sizeof(char));

    int pc = 0;
    while (pc < fsize) 
        pc += disasm(buffer, pc);
    while (head) {
        if (head->addr==offsets[0]) {
            printf("offset_%08Xh:\n", offsets[0]);
            remove_offset();
        }
        printf("%s", head->buf);
        head = head->next;
    }

    return 0;
}

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
                res->mode = 4;  // addressing mode 00 [DISP32]
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
        sprintf(section, "%02X", *(opcode+2));
    else if (disp==4)
        sprintf(section, "%02X%02X%02X%02X", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
    strcat(tempbuf, section);
    memset(section, 0, 50);
}

void save_clean(int n) {
    if (n==1) {
        strcat(tempbuf, section);
        sprintf(tempbuf, "%-25s", tempbuf);
        strcat(buf, tempbuf);
        memset(section, 0, 50);
    }
    else if (n==2) {
        sprintf(section, "%-30s", section);
        strcat(buf, section);
        memset(section, 0, 50);
    }
}

int disasm(unsigned char* buffer, int pc) {
    memset(buf, 0, 100);
    memset(tempbuf, 0, 100);
    memset(section, 0, 50);
    strbuf* node = calloc(1, sizeof(strbuf));
    node->addr = ADDRESS + pc;
    unsigned int operand = 0;
    unsigned char* opcode = &buffer[pc];
    drr* res;
    offset = 0;
    sprintf(buf, "0x%08X:\t", ADDRESS + pc);

    switch (*opcode) {
        case 0x81:    
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE + IMM32;
            //add r/m32, imm32 0x81 /0 id
            if (strcmp(res->reg, "eax")==0) { 
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "add [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "add [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "add %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "add [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "add [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            } 
            //and r/m32, imm32 0x81 /4 id
            else if (strcmp(res->reg, "esp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "and [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "and [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "and %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "and [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "and [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            //cmp r/m32, imm32 0x81 /7 id
            else if (strcmp(res->reg, "edi")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "cmp [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "cmp [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "cmp %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "cmp [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "cmp [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            //or r/m32, imm32 0x81 /1 id
            else if (strcmp(res->reg, "ecx")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "or [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "or [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "or %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "or [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "or [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            //sbb r/m32, imm32 0x81 /3 id
            else if (strcmp(res->reg, "ebx")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "sbb [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "sbb [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "sbb %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "sbb [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "sbb [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            //sub r/m32, imm32 0x81 /5 id
            else if (strcmp(res->reg, "ebp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "sub [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "sub [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "sub %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "sub [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "sub [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            //xor r/m32, imm32 0x81 /6 id
            else if (strcmp(res->reg, "esi")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "xor [%s+0x%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "xor [%s+0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "xor %s, %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "xor [0x%02X%02X%02X%02X], %02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "xor [%s], %02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                save_clean(2);
            }
            else {
                sprintf(section, "db 0x%02X", *opcode);
                save_clean(2);
                operand = 0;
            }
            break;

        case 0xFF:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //call r/m32 0xFF /2
            if (strcmp(res->reg, "edx")==0) {   
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "call [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "call [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "call %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "call [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "call [%s]", res->rm);
                save_clean(2);
            } 
            //dec r/m32 0xFF /1       
            else if (strcmp(res->reg, "ecx")==0) {   
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "dec [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "dec [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "dec %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "dec [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "dec [%s]", res->rm);
                save_clean(2);
            } 
            //inc r/m32 0xFF /0
            else if (strcmp(res->reg, "eax")==0) {   
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "inc [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "inc [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "inc %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "inc [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "inc [%s]", res->rm);
                save_clean(2);
            } 
            //jmp r/m32 0xFF /4
            else if (strcmp(res->reg, "esp")==0) {   
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "jmp [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "jmp [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "jmp %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "jmp [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "jmp [%s]", res->rm);
                save_clean(2);
            } 
            //push r/m32 0xFF /6
            else if (strcmp(res->reg, "esi")==0) {   
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "push [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "push [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "push %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "push [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "push [%s]", res->rm);
                save_clean(2);
            } 
            else {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
            }
            break;

        case 0x0F:
            
            //clflush m8 0x0F 0xAE /7, no 11
            if (*(opcode+1)==0xAE && strcmp(res->reg, "edi")==0) {
                res = decode_MODRM(*(opcode+2));
                sprintf(tempbuf, "%02X%02X%02X", *opcode, *(opcode+1), *(opcode+2));
                if (res->disp==1)
                    sprintf(section, "%02X", *(opcode+3));
                else if (res->disp==4)
                    sprintf(section, "%02X%02X%02X%02X", *(opcode+3),*(opcode+4),*(opcode+5),*(opcode+6));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "clflush [%s+0x%02X]", res->rm, *(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "clflush [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==3) {
                    sprintf(section, "db 0x%02X", *opcode);
                    operand = 0;
                    break;
                }
                else if (res->mode==4)
                    sprintf(section, "clflush [0x%02X%02X%02X%02X]", *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else sprintf(section, "clflush [%s]", res->rm);
                operand = res->disp + MODRMBYTE + 1;
                save_clean(2);
            }
            //imul r32, r/m32 0x0F 0xAF /r
            else if (*(opcode+1)==0xAF) {
                res = decode_MODRM(*(opcode+2));
                sprintf(tempbuf, "%02X%02X%02X", *opcode, *(opcode+1), *(opcode+2));
                if (res->disp==1)
                    sprintf(section, "%02X", *(opcode+3));
                else if (res->disp==4)
                    sprintf(section, "%02X%02X%02X%02X", *(opcode+3),*(opcode+4),*(opcode+5),*(opcode+6));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "imul %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "imul %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==3)
                    sprintf(section, "imul %s, %s", res->reg, res->rm);
                else if (res->mode==4)
                    sprintf(section, "imul %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else sprintf(section, "imul %s, [%s]", res->reg, res->rm);
                operand = res->disp + MODRMBYTE + 1;
                save_clean(2);
            }
            //jz rel32 0x0F 0x84 cd
            else if (*(opcode+1)==0x84) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
                save_clean(1);
                offset = offset | *(opcode+5)<<24 | *(opcode+4)<<16 | *(opcode+3)<<8 | *(opcode+2);
                if (REL32 & offset) offset |= 0xFFFFFFFFLL<<32;
                operand = IMM32;
                offset += ADDRESS+pc+operand+OPBYTE+1;
                sprintf(section, "jz offset_%08Xh", offset);
                insert_offset(offset);
                operand += 1;
                save_clean(2);
            }
            //jnz rel32 0x0F 0x85 cd
            else if (*(opcode+1)==0x85) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2),*(opcode+3),*(opcode+4),*(opcode+5));
                save_clean(1);
                offset = offset | *(opcode+5)<<24 | *(opcode+4)<<16 | *(opcode+3)<<8 | *(opcode+2);
                if (REL32 & offset) offset |= 0xFFFFFFFFLL<<32;
                operand = IMM32;
                offset += ADDRESS+pc+operand+OPBYTE+1;
                sprintf(section, "jnz offset_%08Xh", offset);
                insert_offset(offset);
                operand += 1;
                save_clean(2);
            }
            else {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
            }
            break;

        case 0xF7:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //idiv r/m32 0xF7 /7
            if (strcmp(res->reg, "edi")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "idiv [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "idiv [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "idiv %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "idiv [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "idiv [%s]", res->rm);
                save_clean(2);
            }
            //imul r/m32 0xF7 /5
            else if (strcmp(res->reg, "ebp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "imul [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "imul [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "imul %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "imul [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "imul [%s]", res->rm);
                save_clean(2);
            }
            //mul r/m32 0xF7 /4
            else if (strcmp(res->reg, "esp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "mul [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "mul [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "mul %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "mul [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "mul [%s]", res->rm);
                save_clean(2);
            }
            //neg r/m32 0xF7 /3
            else if (strcmp(res->reg, "ebx")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "neg [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "neg [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "neg %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "neg [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "neg [%s]", res->rm);
                save_clean(2);
            }
            //not r/m32 0xF7 /2
            else if (strcmp(res->reg, "edx")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "not [%s+0x%02X]", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "not [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "not %s", res->rm);
                else if (res->mode==4)
                    sprintf(section, "not [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "not [%s]", res->rm);
                save_clean(2);
            }
            //test r/m32, imm32 0xF7 /0 id
            else if (strcmp(res->reg, "eax")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "test [%s+0x%02X], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
                else if (res->mode==2)
                    sprintf(section, "test [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else if (res->mode==3)
                    sprintf(section, "test %s, 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==4)
                    sprintf(section, "test [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                        *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
                else sprintf(section, "test [%s], 0x%02X%02X%02X%02X", 
                        res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                operand += IMM32;
                save_clean(2);
            }
            else {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
            }
            break;

        case 0xD1:
            res = decode_MODRM(*(opcode+1));
            operand = res->disp + MODRMBYTE;
            //sal r/m32, 1 0xD1 /4
            if (strcmp(res->reg, "esp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "sal [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "sal [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "sal %s, 1", res->rm);
                else if (res->mode==4)
                    sprintf(section, "sal [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "sal [%s], 1", res->rm);
                save_clean(2);
            }
            //sar r/m32, 1 0xD1 /7
            else if (strcmp(res->reg, "edi")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "sar [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "sar [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "sar %s, 1", res->rm);
                else if (res->mode==4)
                    sprintf(section, "sar [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "sar [%s], 1", res->rm);
                save_clean(2);
            }
            //shr r/m32, 1 0xD1 /5
            else if (strcmp(res->reg, "ebp")==0) {
                sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
                print_DISP(res, opcode);
                save_clean(1);
                if (res->mode==1) 
                    sprintf(section, "shr [%s+0x%02X], 1", res->rm, *(opcode+2));
                else if (res->mode==2)
                    sprintf(section, "shr [%s+0x%02X%02X%02X%02X], 1", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else if (res->mode==3)
                    sprintf(section, "shr %s, 1", res->rm);
                else if (res->mode==4)
                    sprintf(section, "shr [0x%02X%02X%02X%02X], 1", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
                else sprintf(section, "shr [%s], 1", res->rm);
                save_clean(2);
            }
            else {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
            }
            break;

        case 0x05:    //add eax, imm32 0x05 id, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "add eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            save_clean(2);
            operand = IMM32;
            break;

        case 0x01:    //add r/m32, r32 0x01 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "add [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "add [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "add %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "add [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "add [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x03:    //add r32, r/m32 0x03 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "add %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "add %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "add %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "add %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "add %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x25:    //and eax, imm32 0x25 id, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "and eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x21:    //and r/m32, r32 0x21 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "and [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "and [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "and %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "and [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "and [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x23:    //and r32, r/m32 0x23 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "and %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "and %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "and %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "and %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "and %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0xE8:    //call rel32 0xE8 cd
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            offset = offset | *(opcode+4)<<24 | *(opcode+3)<<16 | *(opcode+2)<<8 | *(opcode+1);
            if (REL32 & offset) offset |= 0xFFFFFFFFLL<<32;
            operand = IMM32;
            offset += ADDRESS+pc+operand+OPBYTE;
            sprintf(section, "call offset_%08Xh", offset);
            insert_offset(offset);
            save_clean(2);
            break;

        case 0x3D:    //cmp eax, imm32 0x3D id, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "cmp eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x39:    //cmp r/m32, r32 0x39 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "cmp [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "cmp [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "cmp %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "cmp [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "cmp [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x3B:    //cmp r32, r/m32 0x3B /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "cmp %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "cmp %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "cmp %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "cmp %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "cmp %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x48:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec eax");
            save_clean(2);
            break;

        case 0x49:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec ecx");
            save_clean(2);
            break;
        
        case 0x4A:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec edx");
            save_clean(2);
            break;
        
        case 0x4B:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec ebx");
            save_clean(2);
            break;
        
        case 0x4C:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec esp");
            save_clean(2);
            break;
        
        case 0x4D:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec ebp");
            save_clean(2);
            break;
        
        case 0x4E:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec esi");
            save_clean(2);
            break;
        
        case 0x4F:    //dec r32 0x48 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "dec edi");
            save_clean(2);
            break;
        
        case 0x69:    //imul r32, r/m32, imm32 0x69 /r id
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "imul %s, [%s+0x%02X], 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
            else if (res->mode==2)
                sprintf(section, "imul %s, [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else if (res->mode==3)
                sprintf(section, "imul %s, %s, 0x%02X%02X%02X%02X", 
                    res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                sprintf(section, "imul %s, [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X",  
                    res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else sprintf(section, "imul %s, [%s], 0x%02X%02X%02X%02X", 
                res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            operand = res->disp + MODRMBYTE + IMM32;
            save_clean(2);
            break;

        case 0x40:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc eax");
            save_clean(2);
            break;

        case 0x41:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc ecx");
            save_clean(2);
            break;
        
        case 0x42:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc edx");
            save_clean(2);
            break;
        
        case 0x43:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc ebx");
            save_clean(2);
            break;
        
        case 0x44:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc esp");
            save_clean(2);
            break;
        
        case 0x45:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc ebp");
            save_clean(2);
            break;
        
        case 0x46:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc esi");
            save_clean(2);
            break;
        
        case 0x47:    //inc r32 0x40 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "inc edi");
            save_clean(2);
            break;

        case 0xEB:    //jmp rel8 0xEB cb
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            offset = *(opcode+1);
            if (REL8 & offset) offset |= 0xFFFFFFFFFFFFFF<<8;
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            save_clean(1);
            sprintf(section, "jmp offset_%08Xh", offset);
            insert_offset(offset);
            save_clean(2);
            break;

        case 0xE9:    //jmp rel32 0xE9 cd
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            offset |= *(opcode+4)<<24 | *(opcode+3)<<16 | *(opcode+2)<<8 | *(opcode+1);
            if (REL32 & offset) offset |= 0xFFFFFFFFLL<<32;

            operand = IMM32;
            offset += ADDRESS+pc+operand+OPBYTE;

            save_clean(1);
            sprintf(section, "jmp offset_%08Xh", offset);
            insert_offset(offset);
            save_clean(2);
            break;

        case 0x74:    //jz rel8 0x74 cb
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            offset = *(opcode+1);
            if (REL8 & offset) offset |= 0xFFFFFFFFFFFFFF<<8;
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            save_clean(1);
            sprintf(section, "jz offset_%08Xh", offset);
            insert_offset(offset);
            save_clean(2);
            break;

        case 0x75:    //jnz 0x75 cb
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            offset = *(opcode+1);
            if (REL8 & offset) offset |= 0xFFFFFFFFFFFFFF<<8;
            operand = IMM8;
            offset += ADDRESS+pc+operand+OPBYTE;
            save_clean(1);
            sprintf(section, "jnz offset_%08Xh", offset);
            insert_offset(offset);
            save_clean(2);
            break;

        case 0x8D:    //lea r32, m 0x8D /r, no 11
            res = decode_MODRM(*(opcode+1));
            if (res->mode==3) {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
                break;
            }
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "lea %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "lea %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                sprintf(section, "lea %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "lea %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0xB8:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xB9:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov ecx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBA:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov edx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBB:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov ebx, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBC:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov esp, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBD:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov ebp, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBE:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov esi, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xBF:    //mov r32, imm32 0xB8+rd id
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode,*(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "mov edi, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xC7:    //mov r/m32, imm32 0xC7 /0 id
            res = decode_MODRM(*(opcode+1));
            if (strcmp(res->reg, "eax")!=0) {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
                break;
            }
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+2+res->disp),*(opcode+3+res->disp),*(opcode+4+res->disp),*(opcode+5+res->disp));
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "mov [%s+0x%02X], 0x%02X%02X%02X%02X", res->rm, *(opcode+2), *(opcode+6),*(opcode+5),*(opcode+4),*(opcode+3));
            else if (res->mode==2)
                sprintf(section, "mov [%s+0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", 
                    res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else if (res->mode==3)
                sprintf(section, "mov %s, 0x%02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==4)
                sprintf(section, "mov [0x%02X%02X%02X%02X], 0x%02X%02X%02X%02X", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), *(opcode+9),*(opcode+8),*(opcode+7),*(opcode+6));
            else sprintf(section, "mov [%s], 0x%02X%02X%02X%02X", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            operand = res->disp + MODRMBYTE + IMM32;
            save_clean(2);
            break;

        case 0x89:    //mov r/m32, r32 0x89 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "mov [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "mov [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "mov %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "mov [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "mov [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x8B:    //mov r32, r/m32 0x8B /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "mov %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "mov %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "mov %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "mov %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "mov %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0xA5:    //movsd 0xA5, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "movsd");
            save_clean(2);
            break;

        case 0x90:    //nop 0x90, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "nop");
            save_clean(2);
            break;
            
        case 0x0D:    //or eax, imm32 0x0D id, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            sprintf(section, "%02X%02X%02X%02X", *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "or eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x09:    //or r/m32, r32 0x09 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "or [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "or [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "or %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "or [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "or [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x0B:    //or r32, r/m32 0x0B /r 
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "or %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "or %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "or %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "or %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "or %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0xE7:    //out imm8, eax 0xE7 ib, no MODRM
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            save_clean(1);
            sprintf(section, "out 0x%02X, eax", *(opcode+1));
            operand = IMM8;
            save_clean(2);
            break;

        case 0x8F:    //pop r/m32 0x8F /0
            res = decode_MODRM(*(opcode+1));
            if (strcmp(res->reg, "eax")!=0) {
                sprintf(section, "db 0x%02X", *opcode);
                operand = 0;
                save_clean(2);
                break;
            }
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "pop [%s+0x%02X]", res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "pop [%s+0x%02X%02X%02X%02X]", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "pop %s", res->rm);
            else if (res->mode==4)
                sprintf(section, "pop [0x%02X%02X%02X%02X]", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "pop [%s]", res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x58:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop eax");
            save_clean(2);
            break;

        case 0x59:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop ecx");
            save_clean(2);
            break;
        
        case 0x5A:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop edx");
            save_clean(2);
            break;
        
        case 0x5B:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop ebx");
            save_clean(2);
            break;
        
        case 0x5C:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop esp");
            save_clean(2);
            break;
        
        case 0x5D:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop ebp");
            save_clean(2);
            break;
        
        case 0x5E:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop esi");
            save_clean(2);
            break;
        
        case 0x5F:    //pop r32 0x58 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "pop edi");
            save_clean(2);
            break;

        case 0x50:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push eax");
            save_clean(2);
            break;

        case 0x51:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push ecx");
            save_clean(2);
            break;

        case 0x52:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push edx");
            save_clean(2);
            break;

        case 0x53:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push ebx");
            save_clean(2);
            break;

        case 0x54:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push esp");
            save_clean(2);
            break;

        case 0x55:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push ebp");
            save_clean(2);
            break;

        case 0x56:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push esi");
            save_clean(2);
            break;

        case 0x57:    //push r32 0x50 + rd, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "push edi");
            save_clean(2);
            break;

        case 0x68:    //push imm32 0x68 id, no MODRM
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "push 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0xF2:    //repne cmpsd 0xF2 0xA7
            if (*(opcode+1)!=0xA7) {
                sprintf(section, "db 0x%02X", *opcode);
                save_clean(2);
                break;
            }
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            save_clean(1);
            sprintf(section, "repne cmpsd");
            save_clean(2);
            operand = 1;
            break;

        case 0xCB:    //retf 0xCB, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "retf");
            save_clean(2);
            break;

        case 0xCA:    //retf imm16 0xCA iw, no MODRM
            sprintf(tempbuf, "%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2));
            save_clean(1);
            sprintf(section, "retf 0x%02X%02X", *(opcode+2),*(opcode+1));
            operand = IMM16;
            save_clean(2);
            break;

        case 0xC3:    //retn 0xC3, no MODRM
            sprintf(tempbuf, "%02X", *opcode);
            save_clean(1);
            sprintf(section, "retn");
            save_clean(2);
            break;

        case 0xC2:    //retn imm16 0xC2 iw, no MODRM
            sprintf(tempbuf, "%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2));
            save_clean(1);
            sprintf(section, "retn 0x%02X%02X", *(opcode+2),*(opcode+1));
            operand = IMM16;
            save_clean(2);
            break;

        case 0x1D:    //sbb eax, imm32 0x1D id, no MODRM
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "sbb eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x19:    //sbb r/m32, r32 0x19 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "sbb [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "sbb [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "sbb %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "sbb [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "sbb [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x1B:    //sbb r32, r/m32 0x1B /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "sbb %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "sbb %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "sbb %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "sbb %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "sbb %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x2D:    //sub eax, imm32 0x2D id, no MODRM
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "sub eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x29:    //sub r/m32, r32 0x29 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "sub [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "sub [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "sub %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "sub [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "sub [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x2B:    //sub r32, r/m32 0x2B /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "sub %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "sub %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "sub %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "sub %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "sub %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0xA9:    //test eax, imm32 0xA9 id, no MODRM
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "test eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x85:    //test r/m32, r32 0x85 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "test [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "test [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "test %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "test [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "test [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x35:    //xor eax, imm32 0x35 id, no MODRM
            sprintf(tempbuf, "%02X%02X%02X%02X%02X", *opcode, *(opcode+1),*(opcode+2),*(opcode+3),*(opcode+4));
            save_clean(1);
            sprintf(section, "xor eax, 0x%02X%02X%02X%02X", *(opcode+4),*(opcode+3),*(opcode+2),*(opcode+1));
            operand = IMM32;
            save_clean(2);
            break;

        case 0x31:    //xor r/m32, r32 0x31 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "xor [%s+0x%02X], %s", res->rm, *(opcode+2), res->reg);
            else if (res->mode==2)
                sprintf(section, "xor [%s+0x%02X%02X%02X%02X], %s", res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else if (res->mode==3)
                sprintf(section, "xor %s, %s", res->rm, res->reg);
            else if (res->mode==4)
                sprintf(section, "xor [0x%02X%02X%02X%02X], %s", *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2), res->reg);
            else sprintf(section, "xor [%s], %s", res->rm, res->reg);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;

        case 0x33:    //xor r32, r/m32 0x33 /r
            sprintf(tempbuf, "%02X%02X", *opcode, *(opcode+1));
            res = decode_MODRM(*(opcode+1));
            print_DISP(res, opcode);
            save_clean(1);
            if (res->mode==1) 
                sprintf(section, "xor %s, [%s+0x%02X]", res->reg, res->rm, *(opcode+2));
            else if (res->mode==2)
                sprintf(section, "xor %s, [%s+0x%02X%02X%02X%02X]", res->reg, res->rm, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else if (res->mode==3)
                sprintf(section, "xor %s, %s", res->reg, res->rm);
            else if (res->mode==4)
                sprintf(section, "xor %s, [0x%02X%02X%02X%02X]",  res->reg, *(opcode+5),*(opcode+4),*(opcode+3),*(opcode+2));
            else sprintf(section, "xor %s, [%s]", res->reg, res->rm);
            operand = res->disp + MODRMBYTE;
            save_clean(2);
            break;
        
        default:
            sprintf(section, "db 0x%02X", *opcode);
            save_clean(2);
            break;
    }

    strcat(buf, "\n");
    node->buf = calloc(100, sizeof(char));
    strcpy(node->buf, buf);
    if (tail) tail->next = node;
    tail = node;
    if (!head) head = node;
    return OPBYTE+operand;
}

void init_offset() {
    offsets = calloc(OFFSET_POOL, sizeof(long long));
}

int compare(const void *a, const void *b) {
    long long* x = (long long*) a; 
    long long* y = (long long*) b;
    return *x - *y;
}

void insert_offset(long long n) {
    for (int i = 0; i < id; i++) {
        if (offsets[i]==n && n!=0) 
            return;
    }
    offsets[id] = n;
    qsort(offsets, id+1, sizeof(long long), compare);
    id+=1;
}

void remove_offset() {
    for (int i = 0; i < OFFSET_POOL-1; i++) {
        offsets[i] = offsets[i+1];
        if (offsets[i]==0) break;
    }
    id-=1;
}