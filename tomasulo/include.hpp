#ifndef _include_
#define _inlcude_
#include<string>

const std::string op_type[] = {"LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",
                              "LB", "LH", "LW", "LBU", "LHU", "SB", "SH", "SW", "ADDI", "SLTI", "SLTIU",
                              "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD", "SUB", "SLL", "SLT",
                              "SLTU", "XOR", "SRL", "SRA", "OR", "AND", "LI"};

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

template<class datatype>
class Queue {
 private:
   int head, tail, cap;
   datatype *data;

 public:
   Queue() {
      head = tail = 0;
      cap = 5;
      data = new datatype[5];
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
   void push(datatype tmp) {
      if (full()) double_space();
      tail = (tail + 1) % cap;
      data[tail] = tmp;
   }
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
};

#endif