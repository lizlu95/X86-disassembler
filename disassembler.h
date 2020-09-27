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
#define OFFSET_POOL			50

// bit masks
#define MODE 				192
#define REG 				56
#define RM 					7
#define REL8				0x0080
#define REL32				0X000080000000

// return structure used by decode_MODRM function 
typedef struct disp_reg_rm {
	unsigned int disp; 		// displacement size for this instruction
	unsigned int mode;		// addressing mode 
	char* reg;				// register name in REG field
	char* rm;				// register name in R/M field
} drr;

// structure for storing processed instructions
typedef struct strbuf {
	long long addr;			// instruction address
	char* buf;				// the whole line needs to be printed out
	struct strbuf* next;	// next instruction to be printed
} strbuf;

// functions
void init_buf();
int compare(const void *a, const void *b);
void insert_offset(long long n);
void remove_offset();
char* reg_name(int reg);
drr* decode_MODRM(unsigned char modrm);
void print_DISP(drr* res, unsigned char* opcode);
void save_clean(int n);
int disasm(unsigned char* buffer, int pc);

