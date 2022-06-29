#ifndef _core_
#define _core_
#include "include.hpp"
#include "scanner.hpp"

class brach_predictor {
public:
void predict(instruction ins, unsigned int &pc) {
   if (ins.type != JALR) {
      pc_pred = pc_pred + ins.imm - 4;
   }
}
}BP;

// 在这一部分你需要完成的工作：
// 1. 实现一个先进先出的指令队列
// 2. 读取指令并存放到指令队列中
// 3. 准备好下一条issue的指令
// tips: 考虑边界问题（满/空...）
void fetch() {
   if (!myIQ.IQ_out.full()) {
      unsigned int code = get_instruction(pc_pred);
      instruction ins = decode_instruction(code, pc_pred);
      myIQ.IQ_out.push(ins);
      if (is_branch_instruction(ins.type)) {
         BP.predict(ins, pc_pred);
      } else pc_pred += 4;
   }
}

void issue() {
   /*
      在这一部分你需要完成的工作：
      1. 从run_inst_fetch_queue()中得到issue的指令
      2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并修改regfile中相应的值
      2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
      tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
      2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
      而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
      3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
      tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
   */
   if (!myROB.full() && !myIQ.IQ_in.empty()) {
      instruction ins = myIQ.IQ_in.front();
      ROB_node rob_node;
      rob_node.busy = YES;
      rob_node.ready = NO;
      rob_node.dest = ins.rd;
      INS_node ins_node(ins);
      if (is_file_instruction(ins.type)) { // to SLB
         if (!mySLB.full()) {
            myIQ.IQ_out.pop();
            ins_node.ins.pos_in_ROB = myROB.ROB_in.next_pos();
            mySLB.SLB_out.push(ins_node);
            rob_node.ins = ins;
            myROB.ROB_out.push(rob_node);
            if (ins.type == SB || ins.type == SH || ins.type == SW) {
               reg_out[ins.rd].Qj = ins_node.ins.pos_in_ROB;
            }
         }
      } else { //to RS
         if (!myRS.full()) {
            myIQ.IQ_out.pop();
            ins_node.ins.pos_in_ROB = myROB.ROB_in.next_pos();
            myRS.RS_out.insert(ins_node);
            rob_node.ins = ins;
            myROB.ROB_out.push(rob_node);
            if (!(4 <= ins.type && ins.type <= 9)) { // branch
               reg_out[ins.rd].Qj = ins_node.ins.pos_in_ROB;
            }
         }
      }
   }
}

void run_reservation(){
   /*
      在这一部分你需要完成的工作：
      1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
      2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
      3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
      4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
   */
   for (int i = 0; i < RS_size; ++i) {
      INS_node node = myRS.RS_in.data[i];
      if (node.busy == YES && node.Qj == -1 && node.Qk == -1) {
         myRS.RS_out.data->busy = NO;
         myALU.ALU_out = node;
         break;
      }
   }
}

void run_slbuffer(){
   /*
      在这一部分中，由于SLBUFFER的设计较为多样，在这里给出两种助教的设计方案：
      1. 1）通过循环队列，设计一个先进先出的SLBUFFER，同时存储 head1、head2、tail三个变量。
      其中，head1是真正的队首，记录第一条未执行的内存操作的指令；tail是队尾，记录当前最后一条未执行的内存操作的指令。
      而head2负责确定处在head1位置的指令是否可以进行内存操作，其具体实现为在ROB中增加一个head_ensure的变量，每个周期head_ensure做取模意义下的加法，直到等于tail或遇到第一条跳转指令，
      这个时候我们就可以保证位于head_ensure及之前位置的指令，因中间没有跳转指令，一定会执行。因而，只要当head_ensure当前位置的指令是Store、Load指令，我们就可以向slbuffer发信息，增加head2。
      简单概括即对head2之前的Store/Load指令，我们根据判断出ROB中该条指令之前没有jump指令尚未执行，从而确定该条指令会被执行。

      2）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。

      3）此外，在我们的设计中，将SLBUFFER进行内存访问时计算需要访问的地址和对应的读取/存储内存的操作在SLBUFFER中一并实现，
      也可以考虑分成两个模块，该部分的实现只需判断队首的指令是否能执行并根据指令相应执行即可。

      2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。
         
      2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
      然后把结果通知ROB，同时将队头弹出。ROB commit到这条指令时通知Regfile写寄存器。
         
      3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
      条store指令，包括计算地址和写内存，写完后将队头弹出。
   */
   if (mySLB.empty()) return;
   INS_node ins_node = mySLB.SLB_in.front();
   if (ins_node.ins.type == SW || ins_node.ins.type == SH || ins_node.ins.type == SB) { // store
      if (ins_node.busy == YES && ins_node.Qj == -1 && ins_node.Qk == -1) {
         mySLB.change_busy(); // SLB_out.front.busy = NO
         myROB[ins_node.ins.pos_in_ROB].dest = ins_node.Vj + ins_node.ins.imm;
         myROB[ins_node.ins.pos_in_ROB].value = ins_node.Vk;
         myROB[ins_node.ins.pos_in_ROB].ready = YES;
      }
   } else { // load
      if (ins_node.busy && ins_node.Qj == -1) {
         mySLB.SLB_out.pop();
         instruction ins = ins_node.ins;
         myROB[ins.pos_in_ROB].dest = ins.rd;
         myROB[ins.pos_in_ROB].ready = YES;
         unsigned int v;
         switch (ins.type) {
            case LB:
               v = signed_extend(read_memory(ins_node.Vj + ins.imm, 1), 8);
               break;
            case LH:
               v = signed_extend(read_memory(ins_node.Vj + ins.imm, 2), 16);
               break;
            case LW:
               v = signed_extend(read_memory(ins_node.Vj + ins.imm, 4), 32);
               break;
            case LBU:
               v = read_memory(ins_node.Vj + ins.imm, 1);
               break;
            case LHU:
               v = read_memory(ins_node.Vj + ins.imm, 2);
               break;
         }
         myROB[ins_node.ins.pos_in_ROB].value = v;
      }
   }
}

void run_ex() {
   /*
      在这一部分你需要完成的工作：
      根据Reservation Station发出的信息进行相应的计算
      tips: 考虑如何处理跳转指令并存储相关信息
      Store/Load的指令并不在这一部分进行处理
   */
   INS_node ins_node = myALU.ALU_in;
   instruction ins = ins_node.ins;
   myROB[ins.pos_in_ROB].ready = YES;
   unsigned int tmp = 0;
   unsigned int &v = myROB[ins.pos_in_ROB].value, &pc_to = myROB[ins.pos_in_ROB].pc_to;
   unsigned int &flag = myROB[ins.pos_in_ROB].ins.actu_jump;
   switch (ins.type) {
      case LUI: //Load Upper Immediate
         v = ins.imm << 12;
         break;
      case AUIPC: //Add Upper Immediate to PC
         v = pc_in + (ins.imm << 12);
         break;
      case JAL: //Jump and  Link
         v = pc_in + 4;
         pc_to = pc_in + ins.imm - 4;
         break;
      case JALR: //Jump and Link Register
         tmp = pc_in + 4;
         pc_to = ((ins_node.Vj + ins.imm) & ~1) - 4;
         v = tmp;
         break;
      case BEQ: //Branch if Equal
         if (ins_node.Vj == ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case BNE: //Branch if Not Equal
         if (ins_node.Vj != ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case BLT: //Branch if Less Than
         if ((int)ins_node.Vj < (int)ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case BGE: //Branch if Greater Than or Equal
         if ((int)ins_node.Vj >= (int)ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case BLTU: //Branch if Less Than, Unsigned
         if ((unsigned int)ins_node.Vj < (unsigned int)ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case BGEU: //Branch if Greater Than or Equal, Unsigned
         if ((unsigned int)ins_node.Vj >= (unsigned int)ins_node.Vk) {
            pc_to = pc_in + ins.imm - 4;
            flag = 1;
         } else pc_to = pc_in, flag = 0;
         break;
      case ADDI: //Add Immediate
         v = ins_node.Vj + ins.imm;
         break;
      case SLTI: //Set if Less Than Immediate
         v = (int) ins_node.Vj < (int) ins.imm;
         break;
      case SLTIU: //Set if Less Than Immediate, Unsigned
         v = (unsigned int) ins_node.Vj < (unsigned int) ins.imm;
         break;
      case XORI: //Exclusive-OR Immediate
         v = ins_node.Vj ^ ins.imm;
         break;
      case ORI: //OR Immediate
         v = ins_node.Vj | ins.imm;
         break;
      case ANDI: //And Immediate
         v = ins_node.Vj & ins.imm;
         break;
      case SLLI: //Shift Left Logical Immediate
         v = ins_node.Vj << ins.shamt;
         break;
      case SRLI: //Shift Right Logical Immediate
         v = (unsigned int) ins_node.Vj >> ins.shamt;
         break;
      case SRAI: //Shift Right Arithmetic Immediate
         v = signed_extend(ins_node.Vj >> ins.shamt, 32 - ins.shamt);
         break;
      case ADD:
         v = ins_node.Vj + ins_node.Vk;
         break;
      case SUB:
         v = ins_node.Vj - ins_node.Vk;
         break;
      case SLL: //Shift Left Logical
         v = ins_node.Vj << ins_node.Vk;
         break;
      case SLT: //Set if Less Than
         v = (int)ins_node.Vj < (int)ins_node.Vk;
         break;
      case SLTU: //Set if Less Than, Unsigned
         v = (unsigned int)ins_node.Vj < (unsigned int)ins_node.Vk;
         break;
      case XOR: //Exclusive-OR
         v = ins_node.Vj ^ ins_node.Vk;
         break;
      case SRL: //Shift Right Logical
         v = (unsigned int)ins_node.Vj >> ins_node.Vk;
         break;
      case SRA: //Shift Right Arithmetic
         v = signed_extend((unsigned int) ins_node.Vj >> ins_node.Vk, 32 - ins_node.Vk);
         break;
      case OR:
         v = ins_node.Vj | ins_node.Vk;
         break;
      case AND:
         v = ins_node.Vj & ins_node.Vk;
         break;
   }
   if (is_change_reg_instruction(ins.type)) {
      for (int i = 0; i < RS_size; ++i) {
         INS_node &tmp = myRS.RS_out.data[i];
         if (tmp.busy == YES) {
            if (tmp.Qj == ins.pos_in_ROB) {
               tmp.Qj = -1;
               tmp.Vj = v;
            }
            if (tmp.Qk == ins.pos_in_ROB) {
               tmp.Qk = -1;
               tmp.Vk = v;
            }
         }
      }
      for (int i = 0; i < SLB_size; ++i) {
         INS_node &tmp = mySLB[i];
         if (tmp.Qj == ins.pos_in_ROB) {
            tmp.Qj = -1;
            tmp.Vj = v;
         }
         if (tmp.Qk == ins.pos_in_ROB) {
            tmp.Qk = -1;
            tmp.Vk = v;
         }
      }
   }
}

void run_rob(){
/*
   在这一部分你需要完成的工作：
   1. 实现一个先进先出的ROB，存储所有指令
   1. 根据issue阶段发射的指令信息分配空间进行存储。
   2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
   3. 对于队首的指令，如果已经完成计算及更新，进行commit
*/
   if (myROB.empty()) return;
   ROB_node rob_node = myROB.ROB_out.front();
   myROB.ROB_out.pop();
   if (10 <= rob_node.ins.type && rob_node.ins.type <= 14) { // load
      if (reg_in[rob_node.dest].Qj == rob_node.ins.pos_in_ROB) {
         reg_out[rob_node.dest].Qj = -1;
         reg_out[rob_node.dest].value = rob_node.value;
      }
   } else if (15 <= rob_node.ins.type && rob_node.ins.type <= 17) { // store
      mySLB.SLB_out.pop();
      if (rob_node.ins.type == SB) {
         write_memory(rob_node.value, rob_node.dest, 1);
      } else if (rob_node.ins.type == SH) {
         write_memory(rob_node.value, rob_node.dest, 2);
      } else {
         write_memory(rob_node.value, rob_node.dest, 4);
      }
   } else if (rob_node.ins.type == JAL || rob_node.ins.type == JALR) {
      pc_out = rob_node.pc_to;
      if (reg_in[rob_node.dest].Qj == rob_node.ins.pos_in_ROB) {
         reg_out[rob_node.dest].Qj = -1;
         reg_out[rob_node.dest].value = rob_node.value;
      }
   } else if (BEQ <= rob_node.ins.type && rob_node.ins.type <= BGEU) {
      pc_out = rob_node.pc_to;
      if (rob_node.ins.actu_jump != rob_node.ins.pred_jump) {
         pc_pred = pc_out;
         for (int i = 0; i < RS_size; ++i) {
            myRS.RS_out.data[i].busy = NO;
         }
         while (!mySLB.SLB_out.empty()) mySLB.SLB_out.pop();
         while (!myROB.ROB_out.empty()) myROB.ROB_out.pop();
         for (int i = 0; i < reg_num; ++i) {
            reg_out[i].Qj = -1;
         }
      }
   } else {
      if (reg_in[rob_node.dest].Qj == rob_node.ins.pos_in_ROB) {
         reg_out[rob_node.dest].Qj = -1;
         reg_out[rob_node.dest].value = rob_node.value;
      }
   }
}

// void run_commit(){
//    /*
//       在这一部分你需要完成的工作：
//       1. 根据ROB发出的信息更新寄存器的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
//       2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
//    */
// }

#endif