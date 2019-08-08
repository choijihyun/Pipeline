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
	unsigned char ALUOp;	//Rtype = func     lw = 00	  sw= 00		branch = 01
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
	unsigned char zero; //  0 -> address
	unsigned int ALU_result; // ����Ѱ� memory��
};

struct Control control;
struct Reg_Read reg_read;
struct ALU alu;

unsigned int mem[64] = { 0 }; // �޸� 64bits
unsigned int reg[32] = { 0 }; // �������� 32bits

/**************************************/

unsigned int Inst_Fetch(unsigned int read_addr); // IF -> ����������?
void Register_Read(unsigned int read_reg_1, unsigned int read_reg_2); // 
void Control_Signal(unsigned int opcode); // R���� lw,sw���� branch���� Ȯ��
unsigned char ALU_Control_Signal(unsigned char signal);
void ALU_func(unsigned char ALU_control, unsigned int a, unsigned int b); // R -> func , lw/sw -> add  branch -> substract
unsigned int Memory_Access(unsigned char MemWrite, unsigned char MemRead, unsigned int addr, unsigned int write_data); //
void Register_Write(unsigned char RegWrite, unsigned int Write_reg, unsigned int Write_data); //
unsigned int Sign_Extend(unsigned int inst_16);
unsigned int Shift_Left_2(unsigned int inst); // pc+4
unsigned int Add(unsigned int a, unsigned int b);
unsigned int Mux(char signal, unsigned int a_0, unsigned int b_1);
void print_reg_mem(void);

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
	int total_cycle = 0; // cycle Ƚ��

	// register initialization
	/**************************************/
	reg[8] = 41621;
	reg[9] = 41621;
	reg[16] = 40;
	/**************************************/ //�̷������� �ʱ�ȭ�ȴٴ°ǰ�

	// memory initialization
	/**************************************/
	mem[40] = 3578; //�̷���?
	fp = fopen("input_3.txt", "r");
	if (fp == NULL) // ���� �������
	{
		printf("error: file open fail !!\n");
		exit(1);
	}

	while (feof(fp) == 0) //->������ ���̾ƴϸ� 0�� ��ȯ
	{
		fscanf(fp, "%x", &inst);
		mem[pc] = inst; //�޸��� pc�ּҿ� inst����
		pc = pc + 4; // pc�ּ� �÷��ֱ�
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

	print_reg_mem(); //ó�� ���� ������

	printf("\n ***** Processor START !!! ***** \n");

	pc = 0; // pc �ּҴ� 0���� �ʱ�ȭ

	while (pc < 64) //p�ּ� 64��Ʈ �Ǳ�������
	{
		// pc +4
		pc_add_4 = Add(pc, 4);

		// instruction fetch
		inst = Inst_Fetch(pc);
		printf("Instruction = %08x \n", inst); // instruction ���� �� ���


		// instruction decode
		inst_31_26 = inst >> 26; // opcode -> ������������ �����
		inst_25_21 = (inst & 0x03e00000) >> 21; // rs
		inst_20_16 = (inst & 0x001f0000) >> 16; // rt -> load
		inst_15_11 = (inst & 0x0000f800) >> 11; // rd -> Rtype
		inst_15_0 = inst & 0x0000ffff; // address -> load,store,branch
		inst_25_0 = inst & 0x03ffffff; // jump

		//printf("%x, %x, %x, %x, %x, %x", inst_31_26, inst_25_21, inst_20_16, inst_15_11, inst_15_0, inst_25_0);


		// register read -> �������Ϳ��� ���� �ּ� �ΰ� ������ reg_read ����ü�� ����
		Register_Read(inst_25_21, inst_20_16);

		// create control signal -> ���� op�ڵ� ������ �� ���꺰 control sign�� control����ü�� ����
		Control_Signal(inst_31_26);

		//���⼭ jump���� �ƴ��� ���� ��ü �� �˾ƾ���

		// create ALU control signal -> jump�ϋ��� ����
		ALU_control = ALU_Control_Signal(inst); // ���� �����Ұ��� �޾ƿ�!
		//ALU_control�� ������������ ����� -> 4��Ʈ

		// ALU -> ������ �޾ƿ� 4bits¥����
		inst_ext_32 = Sign_Extend(inst_15_0);
		inst_ext_shift = Shift_Left_2(inst_ext_32);

		mux_result = Mux(control.ALUSrc, reg_read.Read_data_2, inst_ext_32);  //MUX���� ���� ������ �����
		ALU_func(ALU_control, reg_read.Read_data_1, mux_result); //���� ���� ������ ���������� �Ұ�����
		//���� �Լ����� alu����ü�� ALU_result�� ���� ���� �����س���

		// memory access 
		// Memory_Access(unsigned char MemWrite, unsigned char MemRead, unsigned int addr, unsigned int write_data); 
		mem_result = Memory_Access(control.MemWrite, control.MemRead, alu.ALU_result, reg_read.Read_data_2);


		// register write -> ��ȣ �������� MemtoReg == 0-> rtype MemtoReg ==1 -> lw
		// Register_Write(unsigned char RegWrite, unsigned int Write_reg, unsigned int Write_data); //
		Register_Write(control.RegWrite,
			Mux(control.RegDst, inst_20_16, inst_15_11),
			Mux(control.MemtoReg,alu.ALU_result, mem_result));
		//MemtoReg��ȣ �а� Rtype���갪 �������Ϳ� �־����� load�� �ּ��� �޸𸮰��� �־�����
		total_cycle++;
		
		pc = Mux(control.Jump,
			Mux(control.Branch&&alu.zero, pc_add_4, Add(inst_ext_shift, pc_add_4)),
			Shift_Left_2(inst_25_0));

	// result
	/********************************/
		printf("PC : %d \n", pc);
		printf("Total cycle : %d \n", total_cycle);
		print_reg_mem();
		/********************************/

		system("pause");
	}

	printf("\n ***** Processor END !!! ***** \n");
	return 0;
}

unsigned int Mux(char signal, unsigned int a_0, unsigned int b_1)
{
	if (signal == 0)
		return a_0;
	else
		return b_1;
}

unsigned int Inst_Fetch(unsigned int read_addr) //inst = Inst_Fetch(pc); -> �޸𸮿� �ִ� pc�ּ��� �޸� ��������
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

unsigned char ALU_Control_Signal(unsigned char signal) //inst ����
{
	if (control.ALUOp == 0) { //sw/ lw = 00
		return 2; //add
	}
	else if (control.ALUOp == 1) {//branch == 01
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
			alu.zero = 1;//zero�ϱ� �´ٴ� ��ȣ�� 1�� ����
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
		//�̰�� ���ϰ�����?
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
