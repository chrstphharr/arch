/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   ECE 4480/5480                                             */
/*   University of Colorado, Colorado Springs                  */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file     */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE 1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x)&0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS
{
    IRD,
    COND1,
    COND0,
    J5,
    J4,
    J3,
    J2,
    J1,
    J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1,
    PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1,
    ADDR2MUX0,
    MARMUX,
    ALUK1,
    ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x) { return (x[IRD]); }
int GetCOND(int *x) { return ((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x) { return ((x[J5] << 5) + (x[J4] << 4) +
                           (x[J3] << 3) + (x[J2] << 2) +
                           (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x) { return (x[LD_MAR]); }
int GetLD_MDR(int *x) { return (x[LD_MDR]); }
int GetLD_IR(int *x) { return (x[LD_IR]); }
int GetLD_BEN(int *x) { return (x[LD_BEN]); }
int GetLD_REG(int *x) { return (x[LD_REG]); }
int GetLD_CC(int *x) { return (x[LD_CC]); }
int GetLD_PC(int *x) { return (x[LD_PC]); }
int GetGATE_PC(int *x) { return (x[GATE_PC]); }
int GetGATE_MDR(int *x) { return (x[GATE_MDR]); }
int GetGATE_ALU(int *x) { return (x[GATE_ALU]); }
int GetGATE_MARMUX(int *x) { return (x[GATE_MARMUX]); }
int GetGATE_SHF(int *x) { return (x[GATE_SHF]); }
int GetPCMUX(int *x) { return ((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x) { return (x[DRMUX]); }
int GetSR1MUX(int *x) { return (x[SR1MUX]); }
int GetADDR1MUX(int *x) { return (x[ADDR1MUX]); }
int GetADDR2MUX(int *x) { return ((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x) { return (x[MARMUX]); }
int GetALUK(int *x) { return ((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x) { return (x[MIO_EN]); }
int GetR_W(int *x) { return (x[R_W]); }
int GetDATA_SIZE(int *x) { return (x[DATA_SIZE]); }
int GetLSHF1(int *x) { return (x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM 0x08000
#define MEM_CYCLES 5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT; /* run bit */
int BUS;     /* value of the bus */

typedef struct System_Latches_Struct
{

    int PC,  /* program counter */
        MDR, /* memory data register */
        MAR, /* memory address register */
        IR,  /* instruction register */
        N,   /* n condition bit */
        Z,   /* z condition bit */
        P,   /* p condition bit */
        BEN; /* ben register */

    int READY; /* ready bit */
               /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

    int REGS[LC_3b_REGS]; /* register file. */

    int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

    int STATE_NUMBER; /* Current State Number - Provided for debugging */
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help()
{
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle()
{

    eval_micro_sequencer();
    cycle_memory();
    eval_bus_drivers();
    drive_bus();
    latch_datapath_values();

    CURRENT_LATCHES = NEXT_LATCHES;

    CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles)
{
    int i;

    if (RUN_BIT == FALSE)
    {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++)
    {
        if (CURRENT_LATCHES.PC == 0x0000)
        {
            RUN_BIT = FALSE;
            printf("Simulator halted\n\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go()
{
    if (RUN_BIT == FALSE)
    {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
        cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE *dumpsim_file, int start, int stop)
{
    int address; /* this is a byte address */

    printf("\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        printf("  0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        fprintf(dumpsim_file, " 0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE *dumpsim_file)
{
    int k;

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%0.4x\n", BUS);
    printf("MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
        printf("%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%0.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
        fprintf(dumpsim_file, "%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE *dumpsim_file)
{
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch (buffer[0])
    {
    case 'G':
    case 'g':
        go();
        break;

    case 'M':
    case 'm':
        scanf("%i %i", &start, &stop);
        mdump(dumpsim_file, start, stop);
        break;

    case '?':
        help();
        break;
    case 'Q':
    case 'q':
        printf("Bye.\n");
        exit(0);

    case 'R':
    case 'r':
        if (buffer[1] == 'd' || buffer[1] == 'D')
            rdump(dumpsim_file);
        else
        {
            scanf("%d", &cycles);
            run(cycles);
        }
        break;

    default:
        printf("Invalid Command\n");
        break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename)
{
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL)
    {
        printf("Error: Can't open micro-code file %s\n", ucode_filename);
        exit(-1);
    }

    /* Read a line for each row in the control store. */
    for (i = 0; i < CONTROL_STORE_ROWS; i++)
    {
        if (fscanf(ucode, "%[^\n]\n", line) == EOF)
        {
            printf("Error: Too few lines (%d) in micro-code file: %s\n",
                   i, ucode_filename);
            exit(-1);
        }

        /* Put in bits one at a time. */
        index = 0;

        for (j = 0; j < CONTROL_STORE_BITS; j++)
        {
            /* Needs to find enough bits in line. */
            if (line[index] == '\0')
            {
                printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
                       ucode_filename, i);
                exit(-1);
            }
            if (line[index] != '0' && line[index] != '1')
            {
                printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
                       ucode_filename, i, j);
                exit(-1);
            }

            /* Set the bit in the Control Store. */
            CONTROL_STORE[i][j] = (line[index] == '0') ? 0 : 1;
            index++;
        }

        /* Warn about extra bits in line. */
        if (line[index] != '\0')
            printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
                   ucode_filename, i);
    }
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory()
{
    int i;

    for (i = 0; i < WORDS_IN_MEM; i++)
    {
        MEMORY[i][0] = 0;
        MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename)
{
    FILE *prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL)
    {
        printf("Error: Can't open program file %s\n", program_filename);
        exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
        program_base = word >> 1;
    else
    {
        printf("Error: Program file is empty\n");
        exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF)
    {
        /* Make sure it fits. */
        if (program_base + ii >= WORDS_IN_MEM)
        {
            printf("Error: Program file %s is too long to fit in memory. %x\n",
                   program_filename, ii);
            exit(-1);
        }

        /* Write the word to memory array. */
        MEMORY[program_base + ii][0] = word & 0x00FF;
        MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
        ii++;
    }

    if (CURRENT_LATCHES.PC == 0)
        CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files)
{
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for (i = 0; i < num_prog_files; i++)
    {
        load_program(program_filename);
        while (*program_filename++ != '\0')
            ;
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int) * CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[])
{
    FILE *dumpsim_file;

    /* Error Checking */
    if (argc < 3)
    {
        printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
               argv[0]);
        exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ((dumpsim_file = fopen("dumpsim", "w")) == NULL)
    {
        printf("Error: Can't open dumpsim file\n");
        exit(-1);
    }

    while (1)
        get_command(dumpsim_file);
}

/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/

/* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   *
   * moved this crap out of my function, dosen't go inside,
   * goes outside
   *
   */
void eval_micro_sequencer()
{
            printf("in micro sequencer\n");

    int next_state;
    if (GetIRD(CURRENT_LATCHES.MICROINSTRUCTION))
    { // if decode get opcode
        next_state = (CURRENT_LATCHES.IR >> 12) & 0x0F;
    }
    else
    { // get next state based on conditionals
        switch (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION))
        {
        case 1: // memory ready
            next_state |= CURRENT_LATCHES.READY << 1;
            break;
        case 2: // branch enabled
            next_state |= CURRENT_LATCHES.BEN << 2;
            break;
        case 3: // address mode, not register
            next_state |= (CURRENT_LATCHES.IR & 0x0800) >> 11;
            break;
        default: // unconditional, i hope we don't have a state 4
            next_state = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);
        }
    }
            printf("before memcpy\n");

    memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER], sizeof(int) * CONTROL_STORE_BITS);
            printf("after memcpy\n");

    NEXT_LATCHES.STATE_NUMBER = next_state;
}

/* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   *
   * instructions go outside homie!
   *
   */

int mem_cycle = 0;
void cycle_memory()
{
            printf("in cycle_memory\n");

    int word_address = CURRENT_LATCHES.MAR >> 1 & 0x7FFF;
    int spot = CURRENT_LATCHES.MAR & 0x01;
    if (GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION))
    { //if memory io enabled
        mem_cycle++;
        if (mem_cycle = MEM_CYCLES)
        { // if current cycle = wait time
            if (GetR_W(CURRENT_LATCHES.MICROINSTRUCTION))
            { // if W set write to memory
                if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION))
                { // if true then its a word not a byte
                    MEMORY[word_address][0] = (unsigned char)(CURRENT_LATCHES.MDR & 0x00FF);
                    MEMORY[word_address][1] = (unsigned char)(CURRENT_LATCHES.MDR & 0xFF00);
                }
                else
                { // else it's a byte, so only write one
                    MEMORY[word_address][spot] = Low16bits((MEMORY[word_address][1] << 8) | MEMORY[word_address][0]);
                }
            }
            else
            { // R is set so read memory into the register
                CURRENT_LATCHES.MDR = Low16bits(MEMORY[word_address][0] + MEMORY[word_address][1] << 8);
            }
        }
        else if (mem_cycle == (MEM_CYCLES - 1))
        { // on pass 4 set ready so we can R/W it next cycle
            CURRENT_LATCHES.READY = 1;
        }
    }
    else
    { // memory IO not enabled, don't wait for memory
        mem_cycle = 0;
        CURRENT_LATCHES.READY = 0;
    }
} // end cycle_memory

/* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */

int gate;
void eval_bus_drivers()
{
    gate = 0;
    if (GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION))
    {                printf("gate = 1\n");

        gate = 1;
    }
    if (GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION))
    {
                printf("gate = 2\n");

        gate = 2;
    }
    if (GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION))
    {                printf("gate =3\n");

        gate = 3;
    }
    if (GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION))
    {                printf("gate = 4\n");

        gate = 4;
    }
    if (GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION))
    {                printf("gate = 5\n");

        gate = 5;
    }
}

int signExtend(int num, int signbit)
{
    if (num && (1 << signbit))
    {
        num = num << (8 * sizeof(int) - 1 - signbit);
        num = num >> (8 * sizeof(int) - 1 - signbit);
    }
    return (num);
}

/* 
     * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */
void drive_bus()
{
    int address1mux = GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int address2mux = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int sr1mux = GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int lshift = GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);
    int aluk = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
    int imm = CURRENT_LATCHES.IR & 0x0020;
    int shiftAmt = CURRENT_LATCHES.IR & 0x0F;
    int dataSize = GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
    int op1, op2, sr1, sr2;

    switch (gate)
    {
    case 1:
    {
        if (GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION))
        {
            op1 = (address1mux) ? CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR >> (6 + (sr1mux * 3))) & 0x07] : CURRENT_LATCHES.PC;
            switch (address2mux)
            {
            case 1:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x003F, 5);
                break;
            }
            case 2:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x01FF, 8);
                break;
            }
            case 3:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x07FF, 10);
                break;
            }
            default:
            {
                op2 = 0;
                break;
            }
            }

            if (lshift)
            {
                op2 = op2 << 1;
            }
            BUS = op1 + op2;
        }
        else
        {
            BUS = (CURRENT_LATCHES.IR & 0x0FF) << 1;
        }
        break;
    }
    case 2:
    {
        BUS = CURRENT_LATCHES.PC;
        break;
    }
    case 3:
    {
        sr1 = (CURRENT_LATCHES.IR >> (6 + (sr1mux * 3))) & 0x07;
        sr2 = (imm) ? signExtend(CURRENT_LATCHES.IR & 0x001F, 4) : (CURRENT_LATCHES.IR & 0x07);
        switch (aluk)
        {
        case 0:
        {
            BUS = (imm) ? Low16bits(CURRENT_LATCHES.REGS[sr1] + sr2) : Low16bits(CURRENT_LATCHES.REGS[sr1] + CURRENT_LATCHES.REGS[sr2]);
            break;
        }
        case 1:
        {
            BUS = (imm) ? Low16bits(CURRENT_LATCHES.REGS[sr1] & sr2) : Low16bits(CURRENT_LATCHES.REGS[sr1] & CURRENT_LATCHES.REGS[sr2]);
            break;
        }
        case 2:
        {
            BUS = (imm) ? Low16bits(CURRENT_LATCHES.REGS[sr1] ^ sr2) : Low16bits(CURRENT_LATCHES.REGS[sr1] ^ CURRENT_LATCHES.REGS[sr2]);
            break;
        }
        default:
        {
            BUS = Low16bits(CURRENT_LATCHES.REGS[sr1]);
            break;
        }
        }
        break;
    }
    case 4:
    {
        sr1 = (CURRENT_LATCHES.IR >> (6 + (sr1mux * 1))) & 0x07;
        if (CURRENT_LATCHES.IR & 0x0010)
        {
            if (CURRENT_LATCHES.IR & 0x0020)
            {
                BUS = Low16bits(signExtend(CURRENT_LATCHES.REGS[sr1], 15) >> shiftAmt);
            }
            else
                BUS = Low16bits(CURRENT_LATCHES.REGS[sr1] >> shiftAmt);
        }
        else
            BUS = Low16bits(CURRENT_LATCHES.REGS[sr1] << shiftAmt);
        break;
    }
    case 5:
    {
        BUS = (dataSize) ? Low16bits(CURRENT_LATCHES.MDR) : Low16bits(signExtend(CURRENT_LATCHES.MDR & 0x00FF, 7));
        break;
    }
    default:
    {
        BUS = 0;
        break;
    }
    }
}

/* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */

void setConditionCodes(int result)
{
    if (result < 0)
    {
        NEXT_LATCHES.N = 1;
        NEXT_LATCHES.Z = 0;
        NEXT_LATCHES.P = 0;
    }
    else if (result > 0)
    {
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 0;
        NEXT_LATCHES.P = 1;
    }
    else
    {
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 1;
        NEXT_LATCHES.P = 0;
    }
}
void latch_datapath_values()
{
    int loadMAR = GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION);
    int loadMDR = GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION);
    int memEN = GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION);
    int dataSize = GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
    int loadIR = GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION);
    int address = CURRENT_LATCHES.MAR >> 1;
    int byte = CURRENT_LATCHES.MAR & 0x01;
    int drMux = GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int loadReg = GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION);
    int sr1mux = GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int address1mux = GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int address2mux = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int leftShift = GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION);
    int pcMux = GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
    int loadPC = GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION);
    int loadCC = GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION);
    int op1, op2;

    if (loadMAR)
        NEXT_LATCHES.MAR = Low16bits(BUS);
    if (loadMDR)
    {
        if (memEN)
        {
            if (CURRENT_LATCHES.READY)
            {
                if (dataSize)
                    NEXT_LATCHES.MDR = MEMORY[address][0] & 0x00FF | (MEMORY[address][1] & 0x00FF) << 8;
                else
                    NEXT_LATCHES.MDR = signExtend(MEMORY[address][byte] & 0x00FF, 7);
                NEXT_LATCHES.READY = 0;
                mem_cycle = 0;
            }
        }
        else
            NEXT_LATCHES.MDR = (dataSize) ? Low16bits(BUS) : BUS & 0x00FF;
    }
    NEXT_LATCHES.IR = (loadIR) ? Low16bits(BUS) : NEXT_LATCHES.IR;
    if (loadReg)
        if (drMux)
            NEXT_LATCHES.REGS[7] = Low16bits(BUS);
        else
            NEXT_LATCHES.REGS[(CURRENT_LATCHES.IR >> 9) & 0x07] = Low16bits(BUS);
    if (loadCC)
        setConditionCodes(signExtend(Low16bits(BUS), 15));
    if (loadPC)
    {
        switch (pcMux)
        {
        case 0:
        {
            NEXT_LATCHES.PC = CURRENT_LATCHES.PC + 2;
            break;
        }
        case 1:
        {
            NEXT_LATCHES.PC = Low16bits(BUS);
            break;
        }
        default:
        {
            op1 = (address1mux) ? CURRENT_LATCHES.REGS[(CURRENT_LATCHES.IR >> (6 + (sr1mux * 3)) & 0x07)] : CURRENT_LATCHES.PC;
            switch (address2mux)
            {
            case 1:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x003F, 5);
                break;
            }
            case 2:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x01FF, 8);
                break;
            }
            case 3:
            {
                op2 = signExtend(CURRENT_LATCHES.IR & 0x07FF, 10);
                break;
            }
            default:
            {
                op2 = 0;
                break;
            }
            }
            op2 = (leftShift) ? op2 << 1 : op2;
            NEXT_LATCHES.PC = op1 + op2;
            break;
        }
        }
    }
}
