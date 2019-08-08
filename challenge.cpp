#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable: 4996)
/**************************************/

struct Control
{
	unsigned char RegDst;	//Rtype = 1(rd)    lw = 0(rt) sw= x			branch = x
	unsigned char Jump;		//Rtype = ?		   lw = ?	  sw= ?			branch = ?
	unsigned char Branch;	//Rtype = 0        lw = 0	  sw= 0			branch = 1
	unsigned char MemRead;	//Rtype = 0        lw = 1	  sw= 0			branch = 0
	unsigned char MemtoReg;	//Rtype = 0        lw = 1	  sw= x 		branch = x
	unsigned char ALUOp;	//Rtype = func     lw = 0	  sw= 0			branch = 1
	unsigned char MemWrite;	//Rtype = 0        lw = 0	  sw= 1			branch = 0
	unsigned char ALUSrc;	//Rtype = 0        lw = 1	  sw= 1			branch = 0
	unsigned char RegWrite;	//Rtype = 1        lw = 1	  sw= 0			branch = 0
};

struct Reg_Read
{
	unsigned int Read_data_1; // rs
	unsigned int Read_data_2; // rt
};

struct ALU
{
	unsigned char zero; // 1: enable, 0: disable
	unsigned int ALU_result;
};



struct WB {
	unsigned char MemtoReg;
	unsigned char RegWrite;
}WB;

struct M {
	unsigned char MemWrite;
	unsigned char MemRead;
	unsigned char alu_zero;
	unsigned char branch;
}M;

struct EX {
	unsigned char RegDst;
	unsigned char ALUOp;
	unsigned char ALUSrc;
}EX;



struct IFID {
	unsigned int pc_add_4;
	unsigned int inst;

}IFID;

struct IDEX {
	/*****control*****/
	struct EX EX;
	struct M M;
	struct WB WB;

	unsigned char jump;
	/*****instruction*****/
	unsigned int inst;
	unsigned int pc_add_4;
	unsigned int reg_read_data1;
	unsigned int reg_read_data2;
	unsigned int sign_ext;
	unsigned int inst_20_16;
	unsigned int inst_15_11;

}IDEX;

struct EXMEM {
	/*****control*****/
	struct M M;
	struct WB WB;

	unsigned char jump;
	/*****instruction******/
	unsigned int inst;
	unsigned int add_result;
	unsigned int alu_result;
	unsigned int reg_read_data2;
	unsigned int mux_result;
	unsigned int alu_zero;
}EXMEM;

struct MEMWB {
	/*****control*****/
	struct WB WB;

	/*****instruction*****/
	unsigned int inst;
	unsigned int read_Data;
	unsigned int alu_result;
	unsigned int mux_result;
}MEMWB;

/**************************구조체******************************/

struct Control control;
struct Reg_Read reg_read;
struct ALU alu;
struct MEMWB memwb;
struct EXMEM exmem;
struct IDEX idex;
struct IFID ifid;

unsigned int mem[64] = { 0 };
unsigned int reg[32] = { 0 };

unsigned int cycle = 0;

/*****************************전역변수***************************/

unsigned int Inst_Fetch(unsigned int read_addr);
void Register_Read(unsigned int read_reg_1, unsigned int read_reg_2);
void Control_Signal(unsigned int opcode);
unsigned char ALU_Control_Signal(unsigned int signal);
void ALU_func(unsigned char ALU_control, unsigned int a, unsigned int b);
unsigned int Memory_Access(unsigned char MemWrite, unsigned char MemRead, unsigned int addr, unsigned int write_data);
void Register_Write(unsigned char RegWrite, unsigned int Write_reg, unsigned int Write_data);
unsigned int Sign_Extend(unsigned int inst_16);
unsigned int Shift_Left_2(unsigned int inst);
unsigned int Add(unsigned int a, unsigned int b);
unsigned int Mux(char signal, unsigned int a_0, unsigned int b_1);
void print_reg_mem(void);
void init();
int check_cycle(unsigned int opcode);
/**************************************/

int main(void)
{
	unsigned int pc = 0;
	FILE *fp;
	unsigned int inst = 0;
	unsigned int inst_31_26 = 0; //op
	unsigned int inst_25_21 = 0; //rs
	unsigned int inst_20_16 = 0; //rt
	unsigned int inst_15_11 = 0; //rd
	unsigned int inst_15_0 = 0;  //address
	unsigned int inst_ext_32 = 0;
	unsigned int inst_ext_shift = 0;
	unsigned int pc_add_4 = 0;
	unsigned int pc_add_inst = 0;
	unsigned int mux_result = 0;
	unsigned char ALU_control = 0;
	unsigned int inst_25_0 = 0; //jump
	unsigned int jump_addr = 0; //jump
	unsigned int mem_result = 0;
	int total_cycle = 0;

	// register initialization
	/**************************************/
	reg[8] = 41621; // $t0
	reg[9] = 41621; // $t1
	reg[16] = 40; // $s0
	/**************************************/
	
	// memory initialization
	/**************************************/
	mem[40] = 3578;
	/**************************************/

	if (!(fp = fopen("input_4.txt", "r")))
	{
		printf("error: file open fail !!\n");
		exit(1);
	}

	while (feof(fp) == false)
	{
		fscanf(fp, "%x", &inst);
		mem[pc] = inst;
		pc = pc + 4;
	}
	/**************************************/

	// control initialization
	/**************************************/
	control.RegDst = 0;
	control.Jump = 0;
	control.Branch = 0;
	control.MemRead = 0;
	control.ALUOp = 0;
	control.MemWrite = 0;
	control.ALUSrc = 0;
	control.RegWrite = 0;
	/**************************************/

	print_reg_mem();

	printf("\n ***** Processor START !!! ***** \n");

	pc = 0;
	init();
	while (true) // 모든 instruction이 끝나면 종료!
	{
		//WB
		Register_Write(memwb.WB.RegWrite, memwb.mux_result, Mux(memwb.WB.MemtoReg, memwb.alu_result, memwb.read_Data));


		/*************control*******/
		//MEM
		memwb.inst = exmem.inst;
		memwb.read_Data = Memory_Access(exmem.M.MemWrite, exmem.M.MemRead, exmem.alu_result, exmem.reg_read_data2);
		//printf("\nmemwb read_data2 : %d\n", exmem.reg_read_data2);
		memwb.alu_result = exmem.alu_result;
		memwb.mux_result = exmem.mux_result;

		//////////////////////////////////////////////////////////////////////
		pc_add_inst = Add(Shift_Left_2((idex.inst << 25) >> 25), (pc_add_4 >> 28) << 28);
		
		pc = Mux(idex.jump, 
			Mux((exmem.M.branch&&exmem.alu_zero), pc_add_4, exmem.add_result)
			,pc_add_inst);
		//pc랑 branch해야함

		/**********control********/
		memwb.WB = exmem.WB;
		/*************************/

		//EX
		exmem.inst = idex.inst;
		exmem.add_result = Add(idex.pc_add_4, Shift_Left_2(idex.sign_ext));

		mux_result = Mux(idex.EX.ALUSrc, idex.reg_read_data2, idex.sign_ext);


		/*ALU FUNCTION!!!!!*/
		//idex.EX.ALUOp = ALU_Control_Signal(idex.inst);
		ALU_func(ALU_Control_Signal(idex.inst), idex.reg_read_data1, mux_result);
		//////////////////////////////////////////////////////////
		exmem.alu_result = alu.ALU_result;
		exmem.alu_zero = alu.zero;
		exmem.jump = idex.jump;

		exmem.reg_read_data2 = idex.reg_read_data2;
		exmem.mux_result = Mux(idex.EX.RegDst, idex.inst_20_16, idex.inst_15_11);
		/*******************control***************/
		exmem.WB = idex.WB;
		exmem.M = idex.M;
		/*****************************************/

	//ID
		idex.inst = ifid.inst;
		idex.pc_add_4 = ifid.pc_add_4;
		inst_31_26 = ifid.inst >> 26;
		inst_25_21 = (ifid.inst & 0x03e00000) >> 21;
		inst_20_16 = (ifid.inst & 0x001f0000) >> 16;
		inst_15_11 = (ifid.inst & 0x0000f800) >> 11;
		inst_15_0 = ifid.inst & 0x0000ffff;
		inst_25_0 = ifid.inst & 0x03ffffff;

		Register_Read(inst_25_21, inst_20_16);
		inst_ext_32 = Sign_Extend(inst_15_0);

		idex.reg_read_data1 = reg_read.Read_data_1;
		idex.reg_read_data2 = reg_read.Read_data_2;
		idex.sign_ext = inst_ext_32;
		idex.inst_20_16 = inst_20_16;
		idex.inst_15_11 = inst_15_11;
		
		//printf("inst_31_26 : %d\n", inst_31_26);
		//if(ifid.inst!=0)
			Control_Signal(inst_31_26);
		/*****************control***************/
		idex.WB.RegWrite = control.RegWrite;
		idex.WB.MemtoReg = control.MemtoReg;

		idex.M.MemRead = control.MemRead;
		idex.M.MemWrite = control.MemWrite;
		idex.M.branch = control.Branch;

		idex.EX.ALUOp = control.ALUOp;
		idex.EX.ALUSrc = control.ALUSrc;
		idex.EX.RegDst = control.RegDst;
		/****************************************/
		idex.jump = control.Jump;

		//IF
		pc_add_4 = Add(pc, 4);
		inst = Inst_Fetch(pc);
		printf("Instruction = %08x \n", inst);

		if (total_cycle == 0) {
			cycle = check_cycle(inst >> 26);
		//	printf("first instruction cycle : %d\n", cycle);
		}

		ifid.pc_add_4 = pc_add_4;
		ifid.inst = inst;
		total_cycle++;

		// result
		/********************************/

		printf("=== ID/EX ===  %08x \n",idex.inst);
		printf("WB - RegWrite: %d, MemtoReg: %d\n",idex.WB.RegWrite, idex.WB.MemtoReg);
		printf("M  - Branch  : %d, MemRead : %d, MemWrite: %d \n",idex.M.branch, idex.M.MemRead, idex.M.MemWrite);
		printf("EX - RegDst  : %d, ALUOp   : %d, ALUSrc  : %d \n",idex.EX.RegDst, idex.EX.ALUOp,idex.EX.ALUSrc);
		printf("=== EX/MEM === %08x \n",exmem.inst);
		printf("WB - RegWrite: %d, MemtoReg: %d\n",exmem.WB.RegWrite, exmem.WB.MemtoReg);
		printf("M  - Branch  : %d, MemRead : %d, MemWrite: %d \n",exmem.M.branch,exmem.M.MemRead,exmem.M.MemWrite);
		printf("=== MEM/WB === %08x \n",memwb.inst);
		printf("WB - RegWrite: %d, MemtoReg: %d \n",memwb.WB.RegWrite, memwb.WB.MemtoReg);
	
		printf("PC : %d \n", pc);
		printf("Total cycle : %d \n", total_cycle);

		print_reg_mem();
		/********************************/
		//printf("idex.inst -> cycle : %d total_cycle = %d ", check_cycle(idex.inst >> 26),total_cycle);
		if (idex.inst!=0) {
			if (check_cycle(idex.inst >> 26) + total_cycle - 2 > cycle) {
				cycle = check_cycle(idex.inst >> 26) + total_cycle - 2;
				//printf("alternate cycle : %d\n", cycle);
			}
		}
		
		
		if (total_cycle == cycle)
			break;
		

		system("pause");
	}

	printf("\n ***** Processor END !!! ***** \n");



	return 0;
}

int check_cycle(unsigned int opcode) {
	if (opcode == 35) { //lw == 35
		return 5;
	}
	else if (opcode == 43) {// sw == 43
		return 4;
	}
	else if (opcode == 4) {//branch == 4 -> 뭘까 어떻게 되는걸까
		return 5;
	}
	else if (opcode == 0) { // Rtype
		return 5;
	}
	else if (opcode == 2) { // jump
		return 2;
	}
	return -1;
}

void init() {
	idex.EX.ALUOp = 0;
	idex.EX.ALUSrc = 0;
	idex.EX.RegDst = 0;
	idex.M.alu_zero = 0;
	idex.M.branch = 0;
	idex.M.MemRead = 0;
	idex.M.MemWrite = 0;
	idex.WB.MemtoReg = 0;
	idex.WB.RegWrite = 0;
	exmem.M.alu_zero = 0;
	exmem.M.branch = 0;
	exmem.M.MemRead = 0;
	exmem.M.MemWrite = 0;
	exmem.WB.MemtoReg = 0;
	exmem.WB.RegWrite = 0;
	memwb.WB.MemtoReg = 0;
	memwb.WB.RegWrite = 0;
}

unsigned int Mux(char signal, unsigned int a_0, unsigned int b_1)
{
	if (signal == 0)
		return a_0;
	else
		return b_1;
}

unsigned int Inst_Fetch(unsigned int read_addr) //inst = Inst_Fetch(pc); -> 메모리에 있는 pc주소의 메모리 가져다줌
{
	return mem[read_addr];
}

void Register_Read(unsigned int read_reg_1, unsigned int read_reg_2) {
	reg_read.Read_data_1 = reg[read_reg_1];
	reg_read.Read_data_2 = reg[read_reg_2];
}

void Control_Signal(unsigned int opcode)
{
	if (opcode == 35) { //lw == 35
		control.RegDst = 0;
		control.RegWrite = 1;
		control.ALUSrc = 1;
		control.MemWrite = 0;
		control.MemRead = 1;
		control.MemtoReg = 1;
		control.ALUOp = 0; // 00
		control.Jump = 0;
		control.Branch = 0;
	}
	else if (opcode == 43) {// sw == 43
		control.RegDst = 0;//x
		control.RegWrite = 0;
		control.ALUSrc = 1;
		control.MemWrite = 1;
		control.MemRead = 0;
		control.MemtoReg = 1;//x
		control.ALUOp = 0; // 00
		control.Jump = 0;
		control.Branch = 0;
	}
	else if (opcode == 4) {//branch == 4
		control.RegDst = 0;//x
		control.RegWrite = 0;
		control.ALUSrc = 0;
		control.MemWrite = 0;
		control.MemRead = 0;
		control.MemtoReg = 1;//x
		control.ALUOp = 1; // 01
		control.Jump = 0;
		control.Branch = 1;
	}
	else if (opcode == 0) { // Rtype
		control.RegDst = 1;
		control.RegWrite = 1;
		control.ALUSrc = 0;
		control.MemWrite = 0;
		control.MemRead = 0;
		control.MemtoReg = 0;
		control.ALUOp = 2; // 10
		control.Jump = 0;
		control.Branch = 0;
	}
	else if (opcode == 2) { // jump
		control.RegDst = 0;//x
		control.RegWrite = 0;
		control.ALUSrc = 0;
		control.MemWrite = 0;
		control.MemRead = 0;
		control.MemtoReg = 0;//x
		control.ALUOp = 0; // 00
		control.Jump = 1;
		control.Branch = 0;
	}
}

unsigned char ALU_Control_Signal(unsigned int signal) //inst 받음
{
	unsigned int signal_inst;
	signal_inst = (signal >> 26);
	if (signal_inst == 43|| signal_inst ==35) { //sw/ lw = 00
		return 2; //add
	}
	else if (signal_inst == 4) {//branch == 01
		return 6; //sub
	}
	else { // Rtype =10
		if ((signal & 0x3f) == 32)
			return 2;
		else if ((signal & 0x3f) == 34)
			return 6;
		else if ((signal & 0x3f) == 36)
			return 0;
		else if ((signal & 0x3f) == 37)
			return 1;
	}
}

void ALU_func(unsigned char ALU_control, unsigned int a, unsigned int b)
{
	if (ALU_control == 2) { //add
		alu.ALU_result = a + b;
		alu.zero = 0;
	}
	else if (ALU_control == 6) { //sub
		alu.ALU_result = a - b;
		if (alu.ALU_result == 0)
			alu.zero = 1;//zero니까 맞다는 신호로 1을 보냄
	}
	else if (ALU_control == 0) { //AND 
		alu.ALU_result = a&b;
		alu.zero = 0;
	}
	else if (ALU_control == 1) { //OR
		alu.ALU_result = a | b;
		alu.zero = 0;
	}
}

unsigned int Memory_Access(unsigned char MemWrite, unsigned char MemRead, unsigned int addr, unsigned int write_data)
{
	if (MemWrite == 0 && MemRead == 1) { // lw
		return mem[addr];
	}
	else if (MemWrite == 1 && MemRead == 0) { // sw
		mem[addr] = write_data;
		//이경우 리턴값없음?
	}
}

void Register_Write(unsigned char RegWrite, unsigned int Write_reg, unsigned int Write_data)
{
	if (RegWrite) {
		reg[Write_reg] = Write_data;
	}
}

unsigned int Sign_Extend(unsigned int inst_16)
{
	unsigned int inst_32 = 0;
	if ((inst_16 & 0x00008000)) // minus
	{
		inst_32 = inst_16 | 0xffff0000;
	}
	else // plus
	{
		inst_32 = inst_16;
	}

	return inst_32;
}

unsigned int Shift_Left_2(unsigned int inst)
{
	return inst << 2;
}

unsigned int Add(unsigned int a, unsigned int b)
{
	return a + b;
}


void print_reg_mem(void)
{
	int reg_index = 0;
	int mem_index = 0;

	printf("\n===================== REGISTER =====================\n");

	for (reg_index = 0; reg_index < 8; reg_index++)
	{
		printf("reg[%02d] = %08d        reg[%02d] = %08d        reg[%02d] = %08d        reg[%02d] = %08d \n",
			reg_index, reg[reg_index], reg_index + 8, reg[reg_index + 8], reg_index + 16, reg[reg_index + 16], reg_index + 24, reg[reg_index + 24]);
	}

	printf("===================== REGISTER =====================\n");

	printf("\n===================== MEMORY =====================\n");

	for (mem_index = 0; mem_index < 32; mem_index = mem_index + 4)
	{
		printf("mem[%02d] = %012d        mem[%02d] = %012d \n",
			mem_index, mem[mem_index], mem_index + 32, mem[mem_index + 32]);
	}
	printf("===================== MEMORY =====================\n");
}
