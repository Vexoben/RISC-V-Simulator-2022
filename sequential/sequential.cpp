#pragma GCC optimize(2)
#include <cstdio>
#include <iostream>
const std::string type[] = {"LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",
                            "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW", "ADDI", "SLTI", "SLTIU",
                            "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD", "SUB", "SLL", "SLT",
                            "SLTU", "XOR", "SRL", "SRA", "OR", "AND", "LI"};

unsigned int reg[32];
unsigned int  pc;
unsigned char mem[5000000];

enum ins_type {
   LUI, 
   AUIPC,
   JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU,
   LB, LH, LW, LBU, LHU,
   SB, SH, SW,
   ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, 
   ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
   LI
};

struct instruction {
   ins_type type;
   unsigned int imm, rs1, rs2, rd, shamt;
};

void input() {
   int c = getchar(); int pos = 0;
   while (c != EOF) {
      while (c != '@' && !isdigit(c) && !isalpha(c) && c != EOF) c = getchar();
      if (c == EOF) break;
      if (c == '@') {
         for (c = getchar(), pos = 0; isdigit(c) || isalpha(c); c = getchar()) {
            pos = (pos << 4) + (isdigit(c) ? c - '0' : c - 'A' + 10);
         }
      } else {
         for (; isdigit(c) || isalpha(c); c = getchar()) {
            mem[pos] = (mem[pos] << 4) + (isdigit(c) ? c - '0' : c - 'A' + 10);
         }
         ++pos;
      }
   }
}

unsigned int get_instruction(int pc) { //注意这边没有传引用
   unsigned int res = 0;
   for (int i = 0; i < 4; ++i) {
      res |= mem[pc++] << (i << 3);
   }
   return res;
}

unsigned int read_memory(int pos, int len) {
   unsigned int res = 0;
   for (int i = 0; i < len; ++i) {
      res |= mem[pos + i] << (i << 3);
   }
   return res;
}

void write_memory(unsigned int value, int pos, int len) {
   for (int i = 0; i < len; ++i) {
      mem[pos + i] = value & 0xFF;
      value >>= 8;
   }
}

int signed_extend(unsigned int c, int bit) {
   return c >> (bit - 1) & 1 ? c | (0xFFFFFFFF >> bit << bit) : c;
}

instruction decode_instruction(unsigned int code) {
   instruction ins;
   if (code == 0x0ff00513) {
      ins.type = LI;
      return ins;
   }
   ins.rd = code >> 7 & 0x1F;
   ins.rs1 = code >> 15 & 0x1F;
   ins.rs2 = code >> 20 & 0x1F;
   switch(code & 0x7F) {
      case 0x37:
         ins.type = LUI;
         ins.imm = signed_extend(code >> 12, 20);
         break;
      case 0x17:
         ins.type = AUIPC;
         ins.imm = signed_extend(code >> 12, 20);
         break;
      case 0x6F:
         ins.type = JAL;
         ins.imm = (code >> 31 & 1) << 20 | (code >> 21 & 0x3FF) << 1 | (code >> 20 & 1) << 11 | (code >> 12 & 0xFF) << 12;
         ins.imm = signed_extend(ins.imm, 21);
         break;
      case 0x67:
         ins.type = JALR;
         ins.imm = signed_extend(code >> 20, 12);
         break;
      case 0x63:
         ins.imm = (code >> 8 & 0xF) << 1 | (code >> 25 & 0x3F) << 5 | (code >> 7 & 1) << 11 | (code >> 31 & 1) << 12; 
         ins.imm = signed_extend(ins.imm, 13);
         switch (code >> 12 & 7) {
            case 0:
               ins.type = BEQ;
               break;
            case 1:
               ins.type = BNE;
               break;
            case 4:
               ins.type = BLT;
               break;
            case 5:
               ins.type = BGE;
               break;
            case 6:
               ins.type = BLTU;
               break;
            case 7:
               ins.type = BGEU;
               break;
         }
         break;
      case 3:
         ins.imm = signed_extend(code >> 20, 12);
         switch (code >> 12 & 7) {
            case 0:
               ins.type = LB;
               break;
            case 1:
               ins.type = LH;
               break;
            case 2:
               ins.type = LW;
               break;
            case 4:
               ins.type = LBU;
               break;
            case 5:
               ins.type = LHU;
               break;
         }
         break;
      case 0x23:
         ins.imm = (code >> 7 & 0x1F) | (code >> 25) << 5;
         ins.imm = signed_extend(ins.imm, 12);
         switch (code >> 12 & 7) {
            case 0:
               ins.type = SB;
               break;
            case 1:
               ins.type = SH;
               break;
            case 2:
               ins.type = SW;
               break;
         }
         break;
      case 0x13:
         ins.imm = signed_extend(code >> 20, 12);
         ins.shamt = ins.rs2;
         switch (code >> 12 & 7) {
            case 0:
               ins.type = ADDI;
               break;
            case 2:
               ins.type = SLTI;
               break;
            case 3:
               ins.type = SLTIU;
               break;
            case 4:
               ins.type = XORI;
               break;
            case 6:
               ins.type = ORI;
               break;
            case 7:
               ins.type = ANDI;
               break;
            case 1:
               ins.type = SLLI;
               break;
            case 5:
               ins.type = (code >> 30 & 1) ? SRAI : SRLI;
               break;
         }
         break;
      case 0x33:
         switch (code >> 12 & 7) {
            case 0:
               ins.type = (code >> 30 & 1) ? SUB : ADD;
               break;
            case 1:
               ins.type = SLL;
               break;
            case 2:
               ins.type = SLT;
               break;
            case 3:
               ins.type = SLTU;
               break;
            case 4:
               ins.type = XOR;
               break;
            case 5:
               ins.type = (code >> 30 & 1) ? SRA : SRL;
               break;
            case 6:
               ins.type = OR;
               break;
            case 7:
               ins.type = AND;
               break;
         }
         break;
   }
   return ins;
}

void run_ins(instruction ins) {
   int tmp;
   switch (ins.type) {
      case LUI: //Load Upper Immediate
         reg[ins.rd] = ins.imm << 12;
         break;
      case AUIPC: //Add Upper Immediate to PC
         reg[ins.rd] = pc + (ins.imm << 12);
         break;
      case JAL: //Jump and  Link
         reg[ins.rd] = pc + 4;
         pc += ins.imm - 4;
         break;
      case JALR: //Jump and Link Register
         tmp = pc + 4;
         pc = ((reg[ins.rs1] + ins.imm) & ~1) - 4;
         reg[ins.rd] = tmp;
         break;
      case BEQ: //Branch if Equal
         if (reg[ins.rs1] == reg[ins.rs2]) {
            pc += ins.imm - 4;
         }
         break;
      case BNE: //Branch if Not Equal
         if (reg[ins.rs1] != reg[ins.rs2]) {
            pc += ins.imm - 4;
         }
         break;
      case BLT: //Branch if Less Than
         if ((int)reg[ins.rs1] < (int)reg[ins.rs2]) {
            pc += ins.imm - 4;
         }         
         break;
      case BGE: //Branch if Greater Than or Equal
         if ((int)reg[ins.rs1] >= (int)reg[ins.rs2]) {
            pc += ins.imm - 4;
         }         
         break;
      case BLTU: //Branch if Less Than, Unsigned
         if ((unsigned int) reg[ins.rs1] < (unsigned int) reg[ins.rs2]) {
            pc += ins.imm - 4;
         }
         break;
      case BGEU: //Branch if Greater Than or Equal, Unsigned
         if ((unsigned int) reg[ins.rs1] >= (unsigned int) reg[ins.rs2]) {
            pc += ins.imm - 4;
         }
         break;
      case LB: //Load Byte
         reg[ins.rd] = signed_extend(read_memory(reg[ins.rs1] + ins.imm, 1), 8);
         break;
      case LH: //Load Halfword
         reg[ins.rd] = signed_extend(read_memory(reg[ins.rs1] + ins.imm, 2), 16);
         break;
      case LW: //Load Word
         reg[ins.rd] = signed_extend(read_memory(reg[ins.rs1] + ins.imm, 4), 32);
         break;
      case LBU: //Load Byte, Unsigned
         reg[ins.rd] = read_memory(reg[ins.rs1] + ins.imm, 1);
         break;
      case LHU: //Load Halfword, Unsigned
         reg[ins.rd] = read_memory(reg[ins.rs1] + ins.imm, 2);
         break;
      case SB: //Store Byte
         write_memory(reg[ins.rs2], reg[ins.rs1] + ins.imm, 1);
         break;
      case SH: //Store Halfword
         write_memory(reg[ins.rs2], reg[ins.rs1] + ins.imm, 2);
         break;
      case SW: //Store Word
         write_memory(reg[ins.rs2], reg[ins.rs1] + ins.imm, 4);
         break;
      case ADDI: //Add Immediate
         reg[ins.rd] = reg[ins.rs1] + ins.imm;
         break;
      case SLTI: //Set if Less Than Immediate
         reg[ins.rd] = (int)reg[ins.rs1] < (int)ins.imm;
         break;
      case SLTIU: //Set if Less Than Immediate, Unsigned
         reg[ins.rd] = (unsigned int)reg[ins.rs1] < (unsigned int) ins.imm;
         break;
      case XORI: //Exclusive-OR Immediate
         reg[ins.rd] = reg[ins.rs1] ^ ins.imm;
         break;
      case ORI: //OR Immediate
         reg[ins.rd] = reg[ins.rs1] | ins.imm;
         break;
      case ANDI: //And Immediate
         reg[ins.rd] = reg[ins.rs1] & ins.imm;
         break;
      case SLLI: //Shift Left Logical Immediate
         reg[ins.rd] = reg[ins.rs1] << ins.shamt;
         break;
      case SRLI: //Shift Right Logical Immediate
         reg[ins.rd] = (unsigned int) reg[ins.rs1] >> ins.shamt;
         break;
      case SRAI: //Shift Right Arithmetic Immediate
         reg[ins.rd] = signed_extend((unsigned int)reg[ins.rs1] >> ins.shamt, 32 - ins.shamt);
         break;
      case ADD:
         reg[ins.rd] = reg[ins.rs1] + reg[ins.rs2];
         break;
      case SUB:
         reg[ins.rd] = reg[ins.rs1] - reg[ins.rs2];
         break;
      case SLL: //Shift Left Logical
         reg[ins.rd] = reg[ins.rs1] << reg[ins.rs2];
         break;
      case SLT: //Set if Less Than
         reg[ins.rd] = (int)reg[ins.rs1] < (int)reg[ins.rs2];
         break;
      case SLTU: //Set if Less Than, Unsigned
         reg[ins.rd] = (unsigned int) reg[ins.rs1] < (unsigned int) reg[ins.rs2];
         break;
      case XOR: //Exclusive-OR
         reg[ins.rd] = reg[ins.rs1] ^ reg[ins.rs2];
         break;
      case SRL: //Shift Right Logical
         reg[ins.rd] = (unsigned int) reg[ins.rs1] >> reg[ins.rs2];
         break;
      case SRA: //Shift Right Arithmetic
         reg[ins.rd] = signed_extend((unsigned int) reg[ins.rs1] >> reg[ins.rs2], 32 - reg[ins.rs2]);
         break;
      case OR:
         reg[ins.rd] = reg[ins.rs1] | reg[ins.rs2];
         break;
      case AND:
         reg[ins.rd] = reg[ins.rs1] & reg[ins.rs2];
         break;
   }
}

int main() {
   input();
   int clk = 0;
   while (1) {
      ++clk;
      unsigned int code = get_instruction(pc);
      instruction ins = decode_instruction(code);
      if (ins.type == LI) break;
      run_ins(ins);
      pc += 4;
      reg[0] = 0;
   }
   std::cout << ((unsigned int) reg[10] & 255u) << std::endl;
   return 0;
}