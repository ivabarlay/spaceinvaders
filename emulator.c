#include "disassembler.c"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct ConditionCodes {
    uint8_t z:1;
    uint8_t s:1;
    uint8_t p:1;
    uint8_t cy:1;
    uint8_t ac:1;
    uint8_t pad:1;

} ConditionCodes;

typedef struct State8080{
   uint8_t a; 
   uint8_t b; 
   uint8_t c; 
   uint8_t d; 
   uint8_t e; 
   uint8_t h; 
   uint8_t l; 
   uint16_t sp;
   uint16_t pc;
   uint8_t *memory;
   struct ConditionCodes cc;
   uint8_t int_enable;
} State8080;

void UnimplementedInstruction(State8080* state){
    // pc will have advanced by one, so undo that
    printf("Error: Unimplemented instruction\n");
    exit(1);
}

uint8_t Parity16(uint16_t number){
    uint16_t res = number;
    res ^= res >> 8;
    res ^= res >> 4;
    res ^= res >> 2;
    res ^= res >> 1;

    return (uint8_t) ((~res) & 1);
}

uint8_t Parity8(uint16_t number){
    uint16_t res = number;
    res ^= res >> 4;
    res ^= res >> 2;
    res ^= res >> 1;

    return (uint8_t) ((~res) & 1);
}

void LogicFlagsA(State8080 *state)
{
	state->cc.cy = state->cc.ac = 0;
	state->cc.z = (state->a == 0);
	state->cc.s = (0x80 == (state->a & 0x80));
	state->cc.p = Parity8(state->a);
}

int Emulate8080p(State8080* state){
    unsigned char *opcode = &state->memory[state -> pc];
    uint32_t answer32;
    uint16_t answer; 
    uint16_t offset;
    uint16_t ret;
    uint8_t x;

    Disassemble8080Op(state->memory, state->pc);    

    state -> pc += 1;

    switch (*opcode) {
        case 0x00: break;
        case 0x01:
                   state->c = opcode[1];
                   state->b = opcode[2];
                   state->pc += 2;
                   break;
        // case 0x02:
        // case 0x03:
        // case 0x04:
        case 0x05: // DCR B
                   x = state->b - 1;
                   state->cc.z = ((x & 0xFF) == 0);
                   state->cc.s = ((x & 0x80) != 0);
                   state->cc.p = Parity8(x & 0xFF);
                   state->b = x;
                   break;
        case 0x06: // MVI B, D8
                   state->b = opcode[1];
                   state->pc += 1;
                   break;
        // case 0x07:
        // case 0x08:
        case 0x09: // DAD B
                   answer32 = (uint32_t) ((state->h << 8) | state->l) + 
                       (uint32_t) ((state->b << 8) | state->c);
                   state->h = (answer32 & 0x0000FF00) >> 8;
                   state->l = (answer32 & 0x000000FF);
                   state->cc.cy = ((answer32 & 0xFFFF0000) > 0);
                   break;
        // case 0x0a:
        // case 0x0b:
        // case 0x0c:
        case 0x0d: // DCR C
                   x = state->c - 1;
                   state->cc.z = ((x & 0xFF) == 0);
                   state->cc.s = ((x & 0x80) != 0);
                   state->cc.p = Parity8(x);
                   state->c = x;
                   break;
        case 0x0e: // MVI C,D8
                   state->c = opcode[1];
                   state->pc += 1;
                   break;
        case 0x0f: // RRC
                   x = state->a;
                   state->a = ((x >> 1) | ((x & 1) << 7));
                   state->cc.cy = (x & 1);
                   break;
        // case 0x10:
        case 0x11: // LXI D,D16
                   state->e = opcode[1];
                   state->d = opcode[2];
                   state->pc += 2;
                   break;
        // case 0x12:
        case 0x13: // INX D
                   state->e++;
                   if(state->e == 0){
                       state->d++;
                   }
                   break;
        // case 0x14:
        // case 0x15:
        // case 0x16:
        // case 0x17:
        // case 0x18:
        case 0x19: // DAD D
                   answer32 = (uint32_t) ((state->h << 8) | state->l) + 
                       (uint32_t) ((state->d << 8) | state->e);
                   state->h = (answer32 & 0x0000FF00) >> 8;
                   state->l = (answer32 & 0x000000FF);
                   state->cc.cy = ((answer32 & 0xFFFF0000) > 0);
                   break;
        case 0x1a: // LDAX D
                   offset = (state->d << 8) | (state->e);
                   state->a = state->memory[offset];
                   break;
        // case 0x1b:
        // case 0x1c:
        // case 0x1d:
        // case 0x1e:
        case 0x1f: // RAR
                   x = state->a;
                   state->a = ((x >> 1) | (state->cc.cy << 7));
                   state->cc.cy = (x & 1);
                   break;
        // case 0x20:
        case 0x21: // LXI H,D16
                   state->l = opcode[1];
                   state->h = opcode[2];
                   state->pc +=2;
                   break;
        // case 0x22:
        case 0x23: // INX H
                   state->l++;
                   if(state->l == 0){
                       state->h++;
                   }
                   break;
        // case 0x24:
        // case 0x25:
        case 0x26: // MVI H,D8
                   state->h = opcode[1];
                   state->pc += 1;
                   break;
        // case 0x27:
        // case 0x28:
        case 0x29: // DAD H
                   answer32 = (uint32_t) ((state->h << 8) | state->l) + 
                       (uint32_t) ((state->h << 8) | state->l);
                   state->h = (answer32 & 0x0000FF00) >> 8;
                   state->l = (answer32 & 0x000000FF);
                   state->cc.cy = ((answer32 & 0xFFFF0000) > 0);
                   break;
        // case 0x2a:
        // case 0x2b:
        // case 0x2c:
        // case 0x2d:
        // case 0x2e:
        case 0x2f: // CMA
                   state->a = ~state->a;
                   break;
        // case 0x30:
        case 0x31: // LXI SP,D16
                   state->sp = (opcode[2] << 8) | (opcode[1]);
                   state->pc += 2;
                   break;
        case 0x32: // STA adr
                   offset = (opcode[2] << 8) | (opcode[1]);
                   state->memory[offset] = state->a;
                   state->pc += 2;
                   break;
        // case 0x33:
        // case 0x34:
        // case 0x35:
        case 0x36: // MVI M,D8
                   offset = (state->h << 8) | (state->l);
                   state->memory[offset] = opcode[1];
                   state->pc += 1;
                   break;
        // case 0x37:
        // case 0x38:
        // case 0x39:
        case 0x3a: // LDA adr
                   offset = (opcode[2] << 8) | (opcode[1]);
                   state->a = state->memory[offset];
                   state->pc += 2;
                   break;
        // case 0x3b:
        // case 0x3c:
        // case 0x3d:
        case 0x3e: // MVI A,D8
                   state->a = opcode[1];
                   state->pc += 1;
                   break;
        // case 0x3f:
        // case 0x40:
        case 0x41: state->b = state->c; break;
        case 0x42: state->b = state->d; break;
        case 0x43: state->b = state->e; break;
        // case 0x44:
        // case 0x45:
        // case 0x46:
        // case 0x47:
        // case 0x48:
        // case 0x49:
        // case 0x4a:
        // case 0x4b:
        // case 0x4c:
        // case 0x4d:
        // case 0x4e:
        // case 0x4f:
        // case 0x50:
        // case 0x51:
        // case 0x52:
        // case 0x53:
        // case 0x54:
        // case 0x55:
        case 0x56: // MOV D,M
                   offset = (state->h << 8) | (state->l);
                   state->d = state->memory[offset];
                   break;
        // case 0x57:
        // case 0x58:
        // case 0x59:
        // case 0x5a:
        // case 0x5b:
        // case 0x5c:
        // case 0x5d:
        case 0x5e: // MOV E,M
                   offset = (state->h << 8) | (state->l);
                   state->e = state->memory[offset];
                   break;
        // case 0x5f:
        // case 0x60:
        // case 0x61:
        // case 0x62:
        // case 0x63:
        // case 0x64:
        // case 0x65:
        case 0x66: // MOV H,M
                   offset = (state->h << 8) | (state->l);
                   state->h = state->memory[offset];
                   break;
        // case 0x67:
        // case 0x68:
        // case 0x69:
        // case 0x6a:
        // case 0x6b:
        // case 0x6c:
        // case 0x6d:
        // case 0x6e:
        case 0x6f: // MOV L,A
                   state->l = state->a;
                   break;
        // case 0x70:
        // case 0x71:
        // case 0x72:
        // case 0x73:
        // case 0x74:
        // case 0x75:
        // case 0x76:
        case 0x77: // MOV M,A
                   offset = (state->h << 8) | (state->l);
                   state->memory[offset] = state->a;
                   break;
        // case 0x78:
        // case 0x79:
        case 0x7a: // MOV A,D
                   state->a = state->d;
                   break;
        case 0x7b: // MOV A,E
                   state->a = state->e;
                   break;
        case 0x7c: // MOV A,H
                   state->a = state->h;
                   break;
        // case 0x7d: 
        case 0x7e: // MOV A,M
                   offset = (state->h << 8) | (state->l);
                   state->a = state->memory[offset];
                   break;
        // case 0x7f:
        case 0x80: // ADD B
                   answer = (uint16_t) state->a + (uint16_t) state->b;

                   // Zero flag
                   if((answer & 0xFF) == 0)
                       state->cc.z = 1;
                   else
                       state->cc.z = 0;

                   // Sign flag: if bit 7 is set, else clear
                   if((answer & 0x80) == 0x80)
                       state-> cc.s = 1;
                   else
                       state->cc.s = 0;

                   // Carry flag
                   if(answer > 0xFF)
                       state->cc.cy = 1;
                   else
                       state->cc.cy = 0;

                   // Parity is handled by a subroutine
                   state->cc.p = Parity16(answer & 0xFF);

                   state->a = answer & 0xFF;
                   break;

        case 0x81:  // ADD C
                   answer = (uint16_t) state->a + (uint16_t) state->c;
                   state->cc.z = ((answer & 0xFF) == 0);
                   state->cc.s = ((answer & 0x80) != 0);
                   state->cc.cy = (answer > 0xFF);
                   state->cc.p = Parity16(answer & 0xFF);
                   state->a = answer & 0xFF;
                   break;
        // case 0x82:
        // case 0x83:
        // case 0x84:
        // case 0x85:
        case 0x86:  // ADD M
                   offset = (state->h<<8) | (state->l);    
                   answer = (uint16_t) state->a + state->memory[offset];    
                   state->cc.z = ((answer & 0xff) == 0);    
                   state->cc.s = ((answer & 0x80) != 0);    
                   state->cc.cy = (answer > 0xff);    
                   state->cc.p = Parity16(answer&0xff);    
                   state->a = answer & 0xff;    
                   break;
        // case 0x87:
        // case 0x88:
        // case 0x89:
        // case 0x8a:
        // case 0x8b:
        // case 0x8c:
        // case 0x8d:
        // case 0x8e:
        // case 0x8f:
        // case 0x90:
        // case 0x91:
        // case 0x92:
        // case 0x93:
        // case 0x94:
        // case 0x95:
        // case 0x96:
        // case 0x97:
        // case 0x98:
        // case 0x99:
        // case 0x9a:
        // case 0x9b:
        // case 0x9c:
        // case 0x9d:
        // case 0x9e:
        // case 0x9f:
        // case 0xa0:
        // case 0xa1:
        // case 0xa2:
        // case 0xa3:
        // case 0xa4:
        // case 0xa5:
        // case 0xa6:
        case 0xa7: // ANA A
                   state->a = state->a & state->a;
                   LogicFlagsA(state);
                   break;
        // case 0xa8:
        // case 0xa9:
        // case 0xaa:
        // case 0xab:
        // case 0xac:
        // case 0xad:
        // case 0xae:
        case 0xaf: // XRA A
                   state->a = state->a ^ state->a;
                   LogicFlagsA(state);
                   break;
        // case 0xb0:
        // case 0xb1:
        // case 0xb2:
        // case 0xb3:
        // case 0xb4:
        // case 0xb5:
        // case 0xb6:
        // case 0xb7:
        // case 0xb8:
        // case 0xb9:
        // case 0xba:
        // case 0xbb:
        // case 0xbc:
        // case 0xbd:
        // case 0xbe:
        // case 0xbf:
        // case 0xc0:
        case 0xc1: // POP B
                   state->c = state->memory[state->sp];
                   state->b = state->memory[state->sp+1];
                   state->sp += 2;
                   break;
        case 0xc2:  // JNZ address
                   if(0 == state->cc.z){
                       state->pc = (opcode[2] << 8) | opcode[1];
                   }
                   else
                       // branch not taken
                       state->pc += 2;
                   break;

        case 0xc3:  // JMP address
                   state->pc = (opcode[2] << 8) | opcode[1];
                   break;
        // case 0xc4: 
        case 0xc5: // PUSH B
                   state->memory[state->sp-1] = state->b;
                   state->memory[state->sp-2] = state->c;
                   state->sp = state->sp - 2;
                   break;
        case 0xc6: // ADI byte
                   answer = (uint16_t) state->a + (uint16_t) opcode[1];    
                   state->cc.z = ((answer & 0xff) == 0);    
                   state->cc.s = ((answer & 0x80) != 0);    
                   state->cc.cy = (answer > 0xff);    
                   state->cc.p = Parity16(answer&0xff);    
                   state->a = answer & 0xff;    
                   break;
        // case 0xc7:
        // case 0xc8:
        case 0xc9: // RET
                   state->pc = state->memory[state->sp] | (state->memory[state->sp+1] << 8);
                   state->sp += 2;
                   break;
        // case 0xca:
        // case 0xcb:
        // case 0xcc:
        case 0xcd: // CALL address
                   ret = state->pc+2;
                   state->memory[state->sp-1] = (ret >> 8) & 0xFF;
                   state->memory[state->sp-2] = (ret & 0xFF);
                   state->sp = state->sp - 2;
                   state->pc = (opcode[2] << 8) | opcode[1];
                   break;
        // case 0xce:
        // case 0xcf:
        // case 0xd0:
        case 0xd1: // POP D
                   state->e = state->memory[state->sp];
                   state->d = state->memory[state->sp+1];
                   state->sp += 2;
                   break;
        // case 0xd2:
        case 0xd3: // OUT D8
                   state->pc += 1;
                   break;
        // case 0xd4:
        case 0xd5: // PUSH D
                   state->memory[state->sp-1] = state->d;
                   state->memory[state->sp-2] = state->e;
                   state->sp = state->sp - 2;
                   break;
        // case 0xd6:
        // case 0xd7:
        // case 0xd8:
        // case 0xd9:
        // case 0xda:
        // case 0xdb:
        // case 0xdc:
        // case 0xdd:
        // case 0xde:
        // case 0xdf:
        // case 0xe0:
        case 0xe1: // POP H
                   state->l = state->memory[state->sp];
                   state->h = state->memory[state->sp+1];
                   state->sp += 2;
                   break;
        // case 0xe2:
        // case 0xe3:
        // case 0xe4:
        case 0xe5: // PUSH H
                   state->memory[state->sp-1] = state->h;
                   state->memory[state->sp-2] = state->l;
                   state->sp = state->sp - 2;
                   break;
        case 0xe6: // ANI byte
                   x = state->a & opcode[1];
                   state->cc.z = (x == 0);
                   state->cc.s = (0x80 == (x & 0x80));    
                   state->cc.p = Parity8(x);    
                   state->cc.cy = 0;           //Data book says ANI clears CY    
                   state->a = x;    
                   state->pc++;
                   break;
        // case 0xe7:
        // case 0xe8:
        // case 0xe9:
        // case 0xea:
        case 0xeb: // XCHG
                   {
                       x = state->h;
                       state->h = state->d;
                       state->d = x;

                       x = state->l;
                       state->l = state->e;
                       state->e = x;
                   }
                   break;
        // case 0xec:
        // case 0xed:
        // case 0xee:
        // case 0xef:
        // case 0xf0:
        case 0xf1: // POP PSW
                   state->a = state->memory[state->sp+1];
                   x = state->memory[state->sp];
                   state->cc.z  = (x >> 6) & 1;
                   state->cc.s  = (x >> 7) & 1;
                   state->cc.p  = (x >> 2) & 1;
                   state->cc.cy = (x & 1);
                   state->cc.ac = (x >> 4) & 1;
                   state->sp += 2;    
                   break;
        // case 0xf2:
        // case 0xf3:
        // case 0xf4:
        case 0xf5: // PUSH PSW
                   state->memory[state->sp-1] = state->a;
                   x = (state->cc.z << 6) | (state->cc.ac << 4) | (state->cc.p << 2) | 
                        (state->cc.cy) | (0x02) & 0xF5; 
                   state->memory[state->sp-2] = x;
                   state->sp += 2;
                   break;
        // case 0xf6:
        // case 0xf7:
        // case 0xf8:
        // case 0xf9:
        // case 0xfa:
        case 0xfb: // EI
                   break;
        // case 0xfc:
        // case 0xfd:
        case 0xfe: // CPI byte
                   x = state->a - opcode[1];    
                   state->cc.z = (x == 0);    
                   state->cc.s = (0x80 == (x & 0x80));    
                   //It isn't clear in the data book what to do with p - had to pick    
                   state->cc.p = Parity8(x);    
                   state->cc.cy = (state->a < opcode[1]);    
                   state->pc++;     
                   break;
        // case 0xff:
    }
    printf("\t");
	printf("%c", state->cc.z ? 'z' : '.');
	printf("%c", state->cc.s ? 's' : '.');
	printf("%c", state->cc.p ? 'p' : '.');
	printf("%c", state->cc.cy ? 'c' : '.');
	printf("%c  ", state->cc.ac ? 'a' : '.');
	printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x PC %04x\n", state->a, state->b, state->c,
				state->d, state->e, state->h, state->l, state->sp, state->pc);
	return 0;
}

void ReadFileIntoMemoryAt(State8080* state, char* filename, uint32_t offset)
{
	FILE *f= fopen(filename, "rb");
	if (f==NULL)
	{
		printf("error: Couldn't open %s\n", filename);
		exit(1);
	}
	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	uint8_t *buffer = &state->memory[offset];
	fread(buffer, fsize, 1, f);
	fclose(f);
}

State8080* Init8080(void){
    State8080* state = calloc(1, sizeof(State8080));
    state->memory = malloc(0x10000); // 16K
    return state;
}

int main(int argc, char *argv[])
{
    int done = 0;
    State8080* state = Init8080();
    ReadFileIntoMemoryAt(state, "roms/invaders.h", 0);
    ReadFileIntoMemoryAt(state, "roms/invaders.g", 0x800);
    ReadFileIntoMemoryAt(state, "roms/invaders.f", 0x1000);
    ReadFileIntoMemoryAt(state, "roms/invaders.e", 0x1800);

    while(done == 0){
        done = Emulate8080p(state);
    }

    return 0;
}
