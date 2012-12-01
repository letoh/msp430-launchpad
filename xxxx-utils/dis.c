#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short word_t;

typedef struct _symRec_t symRec_t;
struct _symRec_t
{
	word_t addr;
	char *label;
	symRec_t *next;
};

typedef struct _disCtxt_t disCtxt_t;
struct _disCtxt_t
{
	word_t addr;
	word_t instr;
	const word_t *cur;
	const word_t *end;
	unsigned char *buf;
	long length;
	symRec_t *symtbl;
};

#define isFmt1(instr)  (((instr) & 0xfc00) == 0x1000)
#define isFmt2(instr)  (((instr) & 0xe000) == 0x2000)
#define isFmt3(instr)  (((instr) & 0xf000) >= 0x4000)

#define fetch(pCtxt)             do{\
	(pCtxt)->instr = *(pCtxt)->cur;\
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
	word_t addr;
	word_t absaddr;
	addsym(pCtxt, 0xc000); // for code start point
	addsym(pCtxt, 0xffc0); // for interrupt vector
	addsym(pCtxt, 0xffe0);
	//addsym(pCtxt, 0xffe4);
	while(pCtxt->cur < pCtxt->end)
	{
		addr = pCtxt->addr;
		fetch(pCtxt);
		if(pCtxt->instr == 0) {} else
		if(isFmt1(pCtxt->instr)) {
			word_t opcode = decode(pCtxt->instr, 7, 0x7);
			word_t admode = decode(pCtxt->instr, 4, 0x3);
			word_t dstreg = decode(pCtxt->instr, 0, 0xf);
			if(opcode == 5 && dstreg == 0) // call
			{
				fetch(pCtxt);
				addsym(pCtxt, addr * ((admode == 1)?1:0) + evalsign(pCtxt->instr,10));
			}
		} else
		if(isFmt2(pCtxt->instr)) {
			word_t offset = decode(pCtxt->instr, 0, 0x3ff);
			absaddr = addr + 2 * (evalsign(offset,10) + 1);
			addsym(pCtxt, absaddr);
		} else
		if(isFmt3(pCtxt->instr)) {
			word_t addr = pCtxt->addr - sizeof(word_t);
			word_t opcode = decode(pCtxt->instr, 12, 0xf);
			word_t srcreg = decode(pCtxt->instr, 8, 0xf);
			word_t asmode = decode(pCtxt->instr, 4, 0x3);
			word_t dstreg = decode(pCtxt->instr, 0, 0xf);
			if(opcode == 4 && dstreg == 0 && (asmode & 0x1)) // mov dst, pc
			{
				fetch(pCtxt);
				addsym(pCtxt, addr * ((asmode == 1)?1:0) + evalsign(pCtxt->instr,10));
			}
		} else
		{ }
	}
}


int dis_operand(disCtxt_t *pCtxt, word_t reg, word_t mode)
{
	switch(reg)
	{
	case 0:
		switch(mode)
		{
		case 0: case 2:
			printf("r%d", reg);
			break;
		case 1:
			fetch(pCtxt);
			printf("0x%04x(r0)", pCtxt->instr);
			break;
		case 3:  // only for As
			fetch(pCtxt);
			printf("#%d", evalsign(pCtxt->instr,16));
			break;
		default: return 1;
		}
		break;
	case 2:
		switch(mode)
		{
		case 0:
			printf("r%d", reg);
			break;
		case 1:
			fetch(pCtxt);
			printf("&0x%04x", pCtxt->instr);
			break;
		case 2: case 3:
			printf("#%d", 1 << mode);
			break;
		default: return 1;
		}
		break;
	case 3:
		switch(mode)
		{
		case 3:
#if 0
			printf("#-1");
			break;
		case 0: case 1: case 2:
			printf("#%d", mode);
#else
		case 0: case 1: case 2:
			printf("#%d", evalsign(mode, 2));
#endif
			break;
		default: return 1;
		}
		break;
	default:
		if(reg > 15) return 1;
		switch(mode)
		{
		case 0:
			printf("r%d", reg);
			break;
		case 1:
			fetch(pCtxt);
			printf("0x%04x(r%d)", pCtxt->instr, reg);
			break;
		case 2:
			printf("@r%d", reg);
			break;
		case 3:
			printf("@r%d+", reg);
			break;
		default: return 1;
		}
		break;
	}
	return 0;
}

const char* fmt1_instr_str[] =
{
	"rrc", "swpb", "rra", "sxt", "push", "call", "reti"
};
int dis_fmt1(disCtxt_t *pCtxt)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	word_t opcode = decode(pCtxt->instr, 7, 0x7);
	word_t byteop = decode(pCtxt->instr, 6, 0x1);
	word_t admode = decode(pCtxt->instr, 4, 0x3);
	word_t dstreg = decode(pCtxt->instr, 0, 0xf);
	symRec_t *sym;
	if(opcode > 6) return 1;
	if(byteop && ((opcode & 0x1) || opcode == 6)) return 1;
	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
	printf("[%04x] %02x %02x | %s%s",
			addr, pCtxt->instr & 0xff, pCtxt->instr >> 8,
			fmt1_instr_str[opcode], byteop?".b":"");
	if(opcode < 6)
	{
		printf("  ");
		dis_operand(pCtxt, dstreg, admode);
		if(dstreg == 0 && admode == 3)
			printf("   ; 0x%04x", pCtxt->instr);
	}
	printf("\n");
	return 0;
}

const char* fmt2_instr_str[] =
{
	"jnz", "jz", "jnc", "jc", "jn", "jge", "jl", "jmp"
};
int dis_fmt2(disCtxt_t *pCtxt)
{
	word_t addr   = pCtxt->addr - sizeof(word_t);
	word_t cond   = decode(pCtxt->instr, 10, 0x7);
	word_t offset = decode(pCtxt->instr, 0, 0x3ff);
	symRec_t *sym;
	//int value = 2 * (((offset & 0x200)?-((~offset & 0x3ff) + 1):offset) + 1);
	int value = 2 * (evalsign(offset,10) + 1);
	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
	printf("[%04x] %02x %02x | %-4s $%+d ; abs(0x%04x)",
			addr, pCtxt->instr & 0xff, pCtxt->instr >> 8,
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
int dis_fmt3(disCtxt_t *pCtxt)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	word_t opcode = decode(pCtxt->instr, 12, 0xf);
	word_t srcreg = decode(pCtxt->instr, 8, 0xf);
	word_t admode = decode(pCtxt->instr, 7, 0x1);
	word_t byteop = decode(pCtxt->instr, 6, 0x1);
	word_t asmode = decode(pCtxt->instr, 4, 0x3);
	word_t dstreg = decode(pCtxt->instr, 0, 0xf);
	symRec_t *sym;

	if(NULL != (sym = getsym(pCtxt, addr)))
		printf("%s:\n", sym->label);
	printf("[%04x] %02x %02x | %s%s  ",
			addr, pCtxt->instr & 0xff, pCtxt->instr >> 8,
			fmt3_instr_str[opcode], byteop?".b":"");

	dis_operand(pCtxt, srcreg, asmode);
	printf(", ");
	dis_operand(pCtxt, dstreg, admode);
	printf("\n");
	return 0;
}

int dis_data(disCtxt_t *pCtxt)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	printf("[%04x] %02x %02x | .word 0x%04x\n",
			addr, pCtxt->instr & 0xff, pCtxt->instr >> 8,
			pCtxt->instr);
	return 0;
}

int dis_pseudo(disCtxt_t *pCtxt)
{
	word_t addr = pCtxt->addr - sizeof(word_t);
	switch(pCtxt->instr)
	{
	case 0x4303: // mov #0, #0 => nop
		printf("[%04x] %02x %02x | nop\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xc232: // bic #8,sr => dint
		printf("[%04x] %02x %02x | dint\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xd232: // bis #8,sr => eint
		printf("[%04x] %02x %02x | eint\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xc312: // bic #1,sr => clrc
		printf("[%04x] %02x %02x | clrc\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xc222: // bic #4,sr => clrn
		printf("[%04x] %02x %02x | clrn\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xc322: // bic #2,sr => clrz
		printf("[%04x] %02x %02x | clrz\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xd312: // bis #1,sr => setc
		printf("[%04x] %02x %02x | setc\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xd222: // bis #4,sr => setn
		printf("[%04x] %02x %02x | setn\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0xd322: // bis #2,sr => setz
		printf("[%04x] %02x %02x | setz\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	case 0x4130: // mov @sp+,pc
		printf("[%04x] %02x %02x | ret\n", addr, pCtxt->instr & 0xff, pCtxt->instr >> 8);
		return 0;
	}
	return 1;
}

void dis(disCtxt_t *pCtxt)
{
	while(pCtxt->cur < pCtxt->end)
	{
		fetch(pCtxt);
		if(pCtxt->instr == 0) {} else
		if(0 == dis_pseudo(pCtxt)) { continue; } else
		if(isFmt1(pCtxt->instr)) { dis_fmt1(pCtxt); } else
		if(isFmt2(pCtxt->instr)) { dis_fmt2(pCtxt); } else
		if(isFmt3(pCtxt->instr)) { dis_fmt3(pCtxt); } else
		{ dis_data(pCtxt); }
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

