// macro values
#define PREFIX				0xF2
#define OPBYTE				1
#define MODRMBYTE			1
#define DISP8				1
#define DISP32				4
#define IMM8				1
#define IMM16				2
#define IMM32				4
#define ADDRESS 			0x0

// bit masks
#define MODE 				192
#define REG 				56
#define RM 					7



// return structure used by decode_MODRM function 
typedef struct disp_reg_rm {
	unsigned int disp; 		// displacement size for this instruction
	unsigned int mode;				// addressing mode 
	char* reg;				// register name in REG field
	char* rm;				// register name in R/M field
} drr;

// functions
void init_offset();
int compare(const void *a, const void *b);
void insert_offset(unsigned long long n);
void remove_offset();
char* reg_name(int reg);
drr* decode_MODRM(unsigned char modrm);
void print_DISP(drr* res, unsigned char* opcode);
unsigned int disasm(unsigned char* buffer, unsigned int pc);

