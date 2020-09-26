# README  

## compile the project:  
gcc disassembler.c -o disasm  

## run the project:  
./disasm -i [input_file_name]  

## supported Intel x86 instructions and corresponding opcodes:  
add eax, imm32: 0x05 id \
add r/m32, imm32: 0x81 /0x81 id \
add r/m32, r32 : 0x01 /r \
add r32, r/m32 : 0x03 /r \
and eax, imm32 : 0x25 id \
and r/m32, imm32 : 0x81 /4 id \
and r/m32, r32: 0x21 /r \
and r32, r/m32: 0x23 /r \
call rel32: 0xE8 cd \
call r/m32: 0xFF /2 \
clflush m8: 0x0F 0xAE /7 \
cmp eax, imm32: 0x3D id \
cmp r/m32, imm32: 0x81 /7 id \
cmp r/m32, r32: 0x39 /r \
cmp r32, r/m32: 0x3B /r \
dec r/m32: 0xFF /1 \
dec r32: 0x48 + rd \
idiv r/m32: 0xF7 /7 \
imul r/m32: 0xF7 /5 \
imul r32, r/m32: 0x0F 0xAF /r \
imul r32, r/m32, imm32: 0x69 /r id \
inc r/m32: 0xFF /0 \
inc r32: 0x40 + rd \
jmp rel8: 0xEB cb \
jmp rel32: 0xE9 cd \
jmp r/m32: 0xFF /4 \
jz rel8: 0x74 cb \
jz rel32: 0x0f 0x84 cd \
jnz rel8: 0x75 cb \
jnz rel32: 0x0f 0x85 cd \
lea r32, m: 0x8D /r \
mov r32, imm32: 0xB8+rd id \
mov r/m32, imm32: 0xC7 /0 id \
mov r/m32, r32: 0x89 /r \
mov r32, r/m32: 0x8B /r \
movsd: 0xA5 \
mul r/m32: 0xF7 /4 \
neg r/m32: 0xF7 /3 \
nop: 0x90 \
not r/m32: 0xF7 /2 \
or eax, imm32: 0x0D id \
or r/m32, imm32: 0x81 /1 id \
or r/m32, r32: 0x09 /r \
or r32, r/m32: 0x0B /r \
out imm8, eax: 0xE7 ib \
pop r/m32: 0x8F /0 \
pop r32: 0x58 + rd \
push r/m32: 0xFF /6 \
push r32: 0x50 + rd \
push imm32: 0x68 id \
repne cmpsd: 0xF2 0xA7 \
retf: 0xCB \
retf imm16: 0xCA iw \
retn: 0xC3 \
retn imm16: 0xC2 iw \
sal r/m32, 1: 0xD1 /4 \
sar r/m32, 1: 0xD1 /7 \
shr r/m32, 1: 0xD1 /5 \
sbb eax, imm32: 0x1D id \
sbb r/m32, imm32: 0x81 /3 id \
sbb r/m32, r32: 0x19 /r \
sbb r32, r/m32: 0x1B /r \
sub eax, imm32: 0x2D id \
sub r/m32, imm32: 0x81 /5 id \
sub r/m32, r32: 0x29 /r \
sub r32, r/m32: 0x2B /r \
test eax, imm32: 0xA9 id \
test r/m32, imm32: 0xF7 /0 id \
test r/m32, r32: 0x85 /r \
xor eax, imm32: 0x35 id \
xor r/m32, imm32: 0x81 /6 id \
xor r/m32, r32: 0x31 /r \
xor r32, r/m32: 0x33 /r 