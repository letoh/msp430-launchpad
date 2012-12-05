#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SHOW_DATA
#define SHOW_DATA 1
#endif

typedef unsigned short word_t;
typedef unsigned char  byte_t;

typedef struct _symRec_t symRec_t;
struct _symRec_t
{
	word_t addr;
	char *label;
	symRec_t *next;
};

typedef union _instr_t instr_t;
union _instr_t {
	struct {
		word_t prefix   : 4;  // 0001
		word_t reserved : 2;
		word_t opcode   : 3;  // 0 ~ 6
		word_t byte     : 1;
		word_t as       : 2;  // addressing mode
		word_t reg      : 4;  // register
	} fmt1;
	struct {
		word_t prefix   : 4;  // 0010, 0011
		word_t cond     : 2;  // 0 ~ 3
		word_t offset   : 10; // pc = pc + 2 * offset
	} fmt2;
	struct {
		word_t opcode   : 4;  // 4 ~ f
		word_t src      : 4;  // source
		word_t ad       : 1;  // dst addressing mode
		word_t byte     : 1;
		word_t as       : 2;  // src addressing mode
		word_t dst      : 4;  // destination
	} fmt3;
};

typedef union _data_t data_t;
union _data_t {
	word_t value;
	byte_t bytes[2];
	instr_t instr;
};

typedef struct _disCtxt_t disCtxt_t;
struct _disCtxt_t
{
	word_t addr;
	const word_t *cur;
	const word_t *end;
	unsigned char *buf;
	long length;
	symRec_t *symtbl;
};

#define isFmt1(instr)  (((instr) & 0xfc00) == 0x1000)
#define isFmt2(instr)  (((instr) & 0xe000) == 0x2000)
#define isFmt3(instr)  (((instr) & 0xf000) >= 0x4000)

#define fetch(pCtxt,data)             do{\
	(data) = *(pCtxt)->cur;\
	++(pCtxt)->cur, (pCtxt)->addr += sizeof(word_t);\
	} while(0);
#define decode(instr,pos,mask)  (((instr) >> (pos)) & (mask))
// ((offset & 0x200)?-((~offset & 0x3ff) + 1):offset)
#define evalsign(val,bit)       (((val) & (1 << ((bit) - 1)))?-((~(val) & ((1 << (bit)) - 1)) + 1):(val))


symRec_t * getsym(disCtxt_t *pCtxt, word_t addr)
{
	symRec_t *curr = pCtxt->symtbl;
	while(curr)
	{
		if(curr->addr == addr) break;
		curr = curr->next;
	}
	return curr;
}

int addsym(disCtxt_t *pCtxt, word_t addr)
{
	char label[16];
	symRec_t *curr = getsym(pCtxt, addr);
	if(curr) return 1;

	curr = (symRec_t *)malloc(sizeof(symRec_t));
	if(NULL == curr) return 1;
	curr->addr = addr;
	sprintf(label, ".L%04X", addr);
	curr->label = strdup(label);
	curr->next = pCtxt->symtbl;
	pCtxt->symtbl = curr;
	return 0;
}

void delsyms(disCtxt_t *pCtxt)
{
	symRec_t *curr;
	while(pCtxt->symtbl)
	{
		curr = pCtxt->symtbl;
		pCtxt->symtbl = curr->next;
		free(curr->label);
		free(curr);
	}
}

void sym(disCtxt_t *pCtxt)
{
	data_t data;
	word_t addr;
	word_t absaddr;
	addsym(pCtxt, 0xc000); // for code start point
	addsym(pCtxt, 0xffc0); // for interrupt vector
	addsym(pCtxt, 0xffe0);
	//addsym(pCtxt, 0xffe4);
	while(pCtxt->cur < pCtxt->end)
	{
		addr = pCtxt->addr;
		fetch(pCtxt, data.value);
		if(data.value == 0) {} else
		if(isFmt1(data.value)) {
			word_t opcode = decode(data.value, 7, 0x7);
			word_t admode = decode(data.value, 4, 0x3);
			word_t dstreg = decode(data.value, 0, 0xf);
			if(opcode == 5 && dstreg == 0) // call
			{
				fetch(pCtxt, data.value);
				addsym(pCtxt, addr * ((admode == 1)?1:0) + evalsign(data.value,10));
			}
		} else
		if(isFmt2(data.value)) {
			word_t offset = decode(data.value, 0, 0x3ff);
			absaddr = addr + 2 * (evalsign(offset,10) + 1);
			addsym(pCtxt, absaddr);
		} else
		if(isFmt3(data.value)) {
			word_t addr = pCtxt->addr - sizeof(word_t);
			word_t opcode = decode(data.value, 12, 0xf);
			word_t srcreg = decode(data.value, 8, 0xf);
			word_t asmode = decode(data.value, 4, 0x3);
			word_t dstreg = decode(data.value, 0, 0xf);
			if(opcode == 4 && dstreg == 0 && (asmode & 0x1)) // mov dst, pc
			{
				fetch(pCtxt, data.value);
				addsym(pCtxt, addr * ((asmode == 1)?1:0) + evalsign(data.value,10));
			}
		} else
		{ }
	}
}


int dis_operand(disCtxt_t *pCtxt, word_t reg, word_t mode, data_t *pData)
{
	switch(reg)
	{
	case 3: return 0;
	case 0:
		switch(mode)
		{
		case 1: case 3:
			fetch(pCtxt, pData->value);
			return 1;
		default: break;
		}
		break;
	case 2:
	default:
		switch(mode)
		{
		case 1:
			fetch(pCtxt, pData->value);
			return 1;
		default: break;
		}
		break;
	}
	return 0;
}

int print_operand(disCtxt_t *pCtxt, word_t reg, word_t mode, data_t data)
{
	switch(reg)
	{
	case 0:
		switch(mode)
		{
		case 1: printf("0x%04x(r0)", data.value); return 0;
		case 3: printf("#%d", evalsign(data.value,16)); return 0;
		default: break;
		}
		break;
	case 2:
		switch(mode)
		{
		case 1: printf("&0x%04x", data.value); return 0;
		case 2: case 3: printf("#%d", 1 << mode); return 0;
		}
		break;
	case 3: printf("#%d", evalsign(mode, 2)); return 0;
	default:
		switch(mode)
		{
		case 1:
			//printf("0x%04x(r%d)", data.value, reg);
			printf("%d(r%d)", evalsign(data.value,16), reg);
			return 0;
		case 2:
			printf("@r%d", reg);
			return 0;
		case 3:
			printf("@r%d+", reg);
			return 0;
		default: break;
		}
		break;
	}
	printf("r%d", reg);
	return 0;
}

const char* fmt1_instr_str[] =
{
	"rrc", "swpb", "rra", "sxt", "push", "call", "reti"
};
int dis_fmt1(disCtxt_t *pCtxt, data_t data)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	word_t opcode = decode(data.value, 7, 0x7);
	word_t byteop = decode(data.value, 6, 0x1);
	word_t admode = decode(data.value, 4, 0x3);
	word_t dstreg = decode(data.value, 0, 0xf);
	symRec_t *sym;
	data_t operand;
	int num = dis_operand(pCtxt, dstreg, admode, &operand);

	if(opcode > 6) return 1;
	if(byteop && ((opcode & 0x1) || opcode == 6)) return 1;
	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
#if SHOW_DATA
	printf("[%04x] %02x %02x", addr, data.bytes[0], data.bytes[1]);
	printf((num)?" %02x %02x":"      ", operand.bytes[0], operand.bytes[1]);
	printf(" | ");
#endif
	printf("%s%s", fmt1_instr_str[opcode], byteop?".b":"");
	if(opcode < 6)
	{
		printf("  ");
		if(opcode == 5) // call
		{
			printf("#0x%04x", operand.value);
			if(NULL != (sym = getsym(pCtxt, operand.value)))
				printf("   ; %s", sym->label);
			else
				printf("   ; undefined function");
		}
		else
		{
			print_operand(pCtxt, dstreg, admode, operand);
			if(dstreg == 0 && admode == 3)
				printf("   ; 0x%04x", operand.value);
		}
	}
	printf("\n");
	return 0;
}

const char* fmt2_instr_str[] =
{
	"jnz", "jz", "jnc", "jc", "jn", "jge", "jl", "jmp"
};
int dis_fmt2(disCtxt_t *pCtxt, data_t data)
{
	word_t addr   = pCtxt->addr - sizeof(word_t);
	word_t cond   = decode(data.value, 10, 0x7);
	word_t offset = decode(data.value, 0, 0x3ff);
	symRec_t *sym;
	//int value = 2 * (((offset & 0x200)?-((~offset & 0x3ff) + 1):offset) + 1);
	int value = 2 * (evalsign(offset,10) + 1);
	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
#if SHOW_DATA
	printf("[%04x] %02x %02x       | ",
			addr, data.bytes[0], data.bytes[1]);
#endif
	printf("%-4s $%+d ; abs(0x%04x)",
			fmt2_instr_str[cond], value, addr + value);
	if(NULL != (sym = getsym(pCtxt, addr + value)))
		printf(", %s", sym->label);
	printf("\n");
	return 0;
}

const char* fmt3_instr_str[] =
{
	NULL, NULL, NULL, NULL,
	"mov", "add", "addc", "subc", "sub", "cmp", "dadd", "bit", "bic", "bis", "xor", "and"
};
int dis_fmt3(disCtxt_t *pCtxt, data_t data)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	word_t opcode = decode(data.value, 12, 0xf);
	word_t srcreg = decode(data.value, 8, 0xf);
	word_t admode = decode(data.value, 7, 0x1);
	word_t byteop = decode(data.value, 6, 0x1);
	word_t asmode = decode(data.value, 4, 0x3);
	word_t dstreg = decode(data.value, 0, 0xf);
	data_t operand1, operand2;
	int num1 = dis_operand(pCtxt, srcreg, asmode, &operand1);
	int num2 = dis_operand(pCtxt, dstreg, admode, &operand2);
	symRec_t *sym;

	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
#if SHOW_DATA
	printf("[%04x] %02x %02x", addr, data.bytes[0], data.bytes[1]);
	if(num1)      printf(" %02x %02x", operand1.bytes[0], operand1.bytes[1]);
	else if(num2) printf(" %02x %02x", operand2.bytes[0], operand2.bytes[1]);
	else          printf("      ");
	printf(" | ");
#endif
	do {
		if(opcode == 4) // mov
		{
			if(srcreg == 1 && asmode == 3) // @sp+
			{
				printf("pop   ");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
			else if(dstreg == 0) // pc
			{
				printf("br   0x%04x", operand1.value);
				//print_operand(pCtxt, srcreg, asmode, operand1);
				break;
			}
		}
		else if(opcode == 5) // add
		{
			if(srcreg == 3)
			{
			   if(asmode == 1) // #1
			   {
					printf("inc%s  ", byteop?".b":"");
					print_operand(pCtxt, dstreg, admode, operand2);
					break;
			   }
			   else if(asmode == 2) // #2
			   {
					printf("incd%s ", byteop?".b":"");
					print_operand(pCtxt, dstreg, admode, operand2);
					break;
			   }
			}
			else if(srcreg == dstreg && asmode == admode)
			{
				printf("rla%s  ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		else if(opcode == 6) // addc
		{
			if(srcreg == 3 && asmode == 0) // #0
			{
				printf("addc%s ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
			else if(srcreg == dstreg && asmode == admode)
			{
				printf("rlc%s  ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		else if(opcode == 7) // subc
		{
			if(srcreg == 3 && asmode == 0) // #0
			{
				printf("sbc%s  ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		else if(opcode == 8) // sub
		{
			if(srcreg == 3)
			{
			   if(asmode == 1) // #1
			   {
					printf("dec%s  ", byteop?".b":"");
					print_operand(pCtxt, dstreg, admode, operand2);
					break;
			   }
			   else if(asmode == 2) // #2
			   {
					printf("decd%s ", byteop?".b":"");
					print_operand(pCtxt, dstreg, admode, operand2);
					break;
			   }
			}
		}
		else if(opcode == 9) // cmp
		{
			if(srcreg == 3 && asmode == 0) // #0
			{
				printf("tst%s  ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		else if(opcode == 10) // dadd
		{
			if(srcreg == 3 && asmode == 0)  // #0
			{
				printf("dadc%s ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		else if(opcode == 14) // xor
		{
			if(srcreg == 3 && asmode == 1)  // #1
			{
				printf("inv%s  ", byteop?".b":"");
				print_operand(pCtxt, dstreg, admode, operand2);
				break;
			}
		}
		printf("%s%s  ", fmt3_instr_str[opcode], byteop?".b":"");
		print_operand(pCtxt, srcreg, asmode, operand1);
		printf(", ");
		print_operand(pCtxt, dstreg, admode, operand2);
		if(srcreg == 0 && asmode == 3)
			printf("   ; #0x%04x", operand1.value);
	} while(0);
	printf("\n");
#if SHOW_DATA
	if(num1 && num2)
		printf("[%04x] %02x %02x\n", addr + 4, operand2.bytes[0], operand2.bytes[1]);
#endif
	return 0;
}

int dis_data(disCtxt_t *pCtxt, data_t data)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
#if SHOW_DATA
	printf("[%04x] %02x %02x       | ",
			addr, data.bytes[0], data.bytes[1]);
#endif
	printf(".word 0x%04x\n", data.value);
	return 0;
}

int dis_pseudo(disCtxt_t *pCtxt, data_t data)
{
#if SHOW_DATA
#define LINE_HDR  "[%04x] %02x %02x       | "
#else
#define LINE_HDR  ""
#endif
	word_t addr = pCtxt->addr - sizeof(word_t);
	switch(data.value)
	{
	case 0x4303: // mov #0, #0 => nop
		printf(LINE_HDR "nop\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xc232: // bic #8,sr => dint
		printf(LINE_HDR "dint\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xd232: // bis #8,sr => eint
		printf(LINE_HDR "eint\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xc312: // bic #1,sr => clrc
		printf(LINE_HDR "clrc\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xc222: // bic #4,sr => clrn
		printf(LINE_HDR "clrn\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xc322: // bic #2,sr => clrz
		printf(LINE_HDR "clrz\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xd312: // bis #1,sr => setc
		printf(LINE_HDR "setc\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xd222: // bis #4,sr => setn
		printf(LINE_HDR "setn\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0xd322: // bis #2,sr => setz
		printf(LINE_HDR "setz\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	case 0x4130: // mov @sp+,pc
		printf(LINE_HDR "ret\n", addr, data.bytes[0], data.bytes[1]);
		return 0;
	}
	return 1;
#undef LINE_HDR
}

void dis(disCtxt_t *pCtxt)
{
	data_t data;
	while(pCtxt->cur < pCtxt->end)
	{
		fetch(pCtxt, data.value);
		if(data.value == 0 || 0 == dis_pseudo(pCtxt, data)) { continue; }
		if(isFmt1(data.value)) { dis_fmt1(pCtxt, data); } else
		if(isFmt2(data.value)) { dis_fmt2(pCtxt, data); } else
		if(isFmt3(data.value)) { dis_fmt3(pCtxt, data); } else
		{ dis_data(pCtxt, data); }
	}
}

int dis_file(const char *fn)
{
	FILE *fp = fopen(fn, "rb");
	disCtxt_t ctxt = {0};
	if(NULL == fp) return 1;

	fseek(fp, 0, SEEK_END);
	ctxt.length = ftell(fp);
	rewind(fp);
	//printf("length: %d\n", ctxt.length);
	ctxt.buf = (unsigned char *)malloc(ctxt.length);
	if(NULL == ctxt.buf) { fclose(fp); return 1; }

	ctxt.addr = 0xc000;
	ctxt.cur = (const word_t *) ctxt.buf;
	ctxt.end = ctxt.cur + (ctxt.length / 2);
	if(fread(ctxt.buf, ctxt.length, 1, fp))
	{
		sym(&ctxt);
		ctxt.addr = 0xc000;
		ctxt.cur = (const word_t *) ctxt.buf;
		dis(&ctxt);
	}

	free(ctxt.buf);
	delsyms(&ctxt);
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc < 2) { fprintf(stderr, "Usage: %s <bin>\n", argv[0]); return -1; }

	return dis_file(argv[1]);
}

