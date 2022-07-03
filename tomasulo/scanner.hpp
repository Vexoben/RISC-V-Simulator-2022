#ifndef _scanner_
#define _scanner_
#include <iostream>
#include "include.hpp"

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

instruction decode_instruction(unsigned int code, unsigned int pc) {
   instruction ins;
   if (code == 0x0ff00513) {
      ins.type = LI;
      ins.rd = ins.rs1 = ins.rs2 = ins.imm = 0;
      ins.pc = pc;
      return ins;
   }
   ins.code = code;
   ins.type == none;
   ins.pc = pc;
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

#endif