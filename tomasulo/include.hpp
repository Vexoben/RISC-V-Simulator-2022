#ifndef _include_
#define _include_
#include<string>

const int RS_size = 100;
const int IQ_size = 32;
const int SLB_size = 32;
const int ROB_size = 64;
const int reg_num = 32;

const std::string op_type[] = {"LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",
                              "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW", "ADDI", "SLTI", "SLTIU",
                              "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD", "SUB", "SLL", "SLT",
                              "SLTU", "XOR", "SRL", "SRA", "OR", "AND", "LI"};

enum ins_type {
   none = -1,
   LUI, 
   AUIPC,
   JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU,
   LB, LH, LW, LBU, LHU,
   SB, SH, SW,
   ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, 
   ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
   LI
};

enum state {
   YES, NO
};

struct instruction {
   ins_type type;
   unsigned int imm, rs1, rs2, rd, shamt, pc, pos_in_ROB, pred_jump, actu_jump;
};

struct regfile {
   unsigned int value;
   int Qj;
   regfile() {
      Qj = -1;
   }
}reg_in[reg_num], reg_out[reg_num];

unsigned int pc_in, pc_out, pc_pred;
unsigned char mem[5000000];

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

bool is_branch_instruction(ins_type tp) {
   if (tp == AUIPC || tp == JAL || tp == JALR || tp == BEQ || tp == BNE 
      || tp == BLT || tp == BGE || tp == BLTU || tp == BGEU) {
      return 1;
   } else return 0;
}

bool is_file_instruction(ins_type tp) {
   return LB <= tp && tp <= SW;
}

bool is_change_reg_instruction(ins_type tp) {
   return tp <= JALR || (LB <= tp && tp <= LHU) || (tp >= ADDI);
}

template<class datatype, int capcity = 100>
class Queue {
 public:
   int head, tail, cap;

   void double_space() {
      datatype *area = new datatype[cap * 2];
      int i, tmp;
      for (i = 1, tmp = (head + 1) % cap;; ++i) {
         area[i] = data[tmp];
         if (tmp == tail) break;
         tmp = (tmp + 1) % cap;
      }
      head = 0; tail = i;
      cap *= 2;
      delete []data;
      data = area;
   }
 public:
   datatype *data;
   Queue() {
      head = tail = 0;
      cap = capcity;
      data = new datatype[capcity];
   }
   ~Queue() {
      delete []data;
   }
   bool empty() {
      return head == tail;
   }
   bool full() {
      return (tail + 1) % cap == head;
   }
   datatype front() {
      return data[(head + 1) % cap];
   }
   void pop() {
      head = (head + 1) % cap;
   }
   int push(datatype tmp) {
      if (full()) double_space();
      tail = (tail + 1) % cap;
      data[tail] = tmp;
      return tail;
   }
   void operator = (const Queue &obj) {
      delete []data;
      data = new datatype[obj.cap];
      cap = obj.cap;
      head = obj.head;
      tail = obj.tail;
      for (int i = 0; i < cap; ++i) {
         data[i] = obj.data[i];
      }
   }
   int next_pos() {
      return (tail + 1) % cap;
   }
};

class IQ {
 public:
   Queue<instruction, IQ_size> IQ_in, IQ_out;
   void update() {
      IQ_in = IQ_out;
   }
}myIQ;

class INS_node {
 public:
   state busy;
   int Vj, Vk, Qj, Qk; // rs1, rs2
   instruction ins;

   void modifyVQ(int &V, int &Q, int rs) {
      if (reg_in[rs].Qj != -1) {
         Q = reg_in[rs].Qj;
      } else {
         Q = -1;
         V = reg_in[rs].value;
      }
   }

   INS_node() {
      busy = NO;
   }

   INS_node(instruction _ins) {
      ins = _ins;
      if (0 <= ins.type && ins.type <= 2) { // LUI, AUIPC, JAL
      } else if (ins.type == 3) { // JALR
         modifyVQ(Vj, Qj, ins.rs1);
      } else if (4 <= ins.type && ins.type <= 9) { // BEQ, BNE, BLT, BGE, BLTU, BGEU
         modifyVQ(Vj, Qj, ins.rs1);
         modifyVQ(Vk, Qk, ins.rs2);
      } else if (10 <= ins.type && ins.type <= 14) { // LB, LH, LW, LBU, LHU
         modifyVQ(Vj, Qj, ins.rs1);
      } else if (15 <= ins.type && ins.type <= 17) { // SB, SH, SW,
         modifyVQ(Vj, Qj, ins.rs1);
         modifyVQ(Vk, Qk, ins.rs2);
      } else if (18 <= ins.type && ins.type <= 26) { // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
         modifyVQ(Vj, Qj, ins.rs1);
      } else if (27 <= ins.type && ins.type <= 37) {
         modifyVQ(Vj, Qj, ins.rs1);
         modifyVQ(Vk, Qk, ins.rs2);         
      }
      busy = YES;
   }
};

class RS {
 public:
   class rs {
    public:
      INS_node data[RS_size];

      bool insert(INS_node node) {
         for (int i = 0; i < RS_size; ++i) {
            if (data[i].ins.type == none) {
               data[i] = node;
               return 0;
            }
         }
         return 1;
      }
   }RS_in, RS_out;

   bool full() {
      for (int i = 0; i < RS_size; ++i) {
         if (RS_in.data[i].ins.type == none) {
            return 0;
         }
      }
      return 1;
   }

   void update() {
      RS_in = RS_out;
   }
}myRS;

class ALU {
 public:
   INS_node ALU_in, ALU_out;
   void update() {
      ALU_in = ALU_out;
   }
}myALU;

class SLB {
 public:
   Queue<INS_node> SLB_in, SLB_out;
   bool empty() {
      return SLB_in.empty();
   }
   bool full() {
      return SLB_in.full();
   }
   void change_busy() {
      SLB_out.data[(SLB_out.head + 1) % SLB_out.cap].busy = NO;
   }
   void update() {
      SLB_in = SLB_out;
   }
   INS_node& operator [] (int pos) {
      return SLB_out.data[pos];
   }
}mySLB;

class ROB_node {
 public:
   state busy, ready;
   int dest;
   unsigned int value, pc_to;
   instruction ins;
};

class ROB {
 public:
   Queue<ROB_node, 64> ROB_in, ROB_out;
   bool empty() {
      return ROB_in.empty();
   }
   bool full() {
      return ROB_in.full();
   }
   void update() {
      ROB_in = ROB_out;
   }
   ROB_node& operator [] (int pos) {
      return ROB_out.data[pos];
   }
}myROB;


#endif