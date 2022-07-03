#ifndef _include_
#define _include_
#define debug(x) std::cout << #x << ": " << (x) << std::endl;
#include<string>
#include<cassert>

const int RS_size = 100;
const int IQ_size = 32;
const int SLB_size = 32;
const int ROB_size = 32;
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
   unsigned int imm, rs1, rs2, rd, shamt, pc, pos_in_ROB;
   state pred_jump, actu_jump;
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
   std::cout << "read_momory" << pos << ' ' << len << std::endl;
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
   if (tp == JAL || tp == JALR || tp == BEQ || tp == BNE 
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

template<class datatype, int capacity>
class Queue {
 public:
   int head, tail, cap;

 public:
   datatype data[capacity];
   Queue() {
      cap = capacity;
      head = tail = 0;
   }
   ~Queue() {}
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
      tail = (tail + 1) % cap;
      data[tail] = tmp;
      return tail;
   }
   void operator = (const Queue &obj) {
      assert(cap == obj.cap);
      head = obj.head;
      tail = obj.tail;
      for (int i = 0; i < cap; ++i) {
         data[i] = obj.data[i];
      }
   }
   datatype& operator [] (const int pos) {
      return data[pos];
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
   unsigned int Vj, Vk;
   int Qj, Qk; // rs1, rs2
   instruction ins;

   INS_node() {
      busy = NO;
      Qj = Qk = -1;
      ins.type = none;
   }

   INS_node(instruction _ins) {
      ins = _ins;
      Qj = Qk = -1;
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
            if (data[i].busy == NO) {
               data[i] = node;
               return 0;
            }
         }
         return 1;
      }
   }RS_in, RS_out;

   bool full() {
      for (int i = 0; i < RS_size; ++i) {
         if (RS_in.data[i].busy == NO) {
            return 0;
         }
      }
      return 1;
   }

   void update() {
      RS_in = RS_out;
   }

   void output() {
      puts("RS state");
      for (int i = 0; i < RS_size; ++i) {
         if (RS_in.data[i].busy == YES) {
            std::cout << op_type[RS_in.data[i].ins.type] << ' ' << RS_in.data[i].Qj << ' ' << RS_in.data[i].Qk << std::endl;
         }
      }
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
   Queue<INS_node, SLB_size> SLB_in, SLB_out;
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
   void output() {
      puts("SLB OUTPUT");
     debug(SLB_out.head);
      Queue<INS_node, SLB_size> tmp = SLB_out;
      while (!tmp.empty()) {
         INS_node ins_node = tmp.front(); tmp.pop();
         std::cout << ins_node.busy << ' ' << ins_node.ins.type << std::endl;
      }
   }
}mySLB;

class ROB_node {
 public:
   state busy, ready;
   int dest;
   unsigned int value, pc_to;
   instruction ins;

   ROB_node() {
      busy = NO;
      ready = NO;
   }
};

class ROB {
 public:
   Queue<ROB_node, ROB_size> ROB_in, ROB_out;
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
