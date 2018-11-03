/*
    Copyright 2018 Joel Svensson	svenssonjoel@yahoo.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

/*
 This program is based on the "Write a shell in C" tutorial by Stephen Brennan that
 can be found here: https://brennan.io/2015/01/16/write-a-shell-in-c/
 */


#include "platform.h"
#include "xil_printf.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_cache_l.h"
#include "xil_io.h"

#include "xil_exception.h"
#include "xscugic.h"
#include "xgpio.h"
#include "xtime_l.h"
#include "xtmrctr.h"

#include "xdevcfg.h"

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

/* ************************************************************
 * CONFIGURATION
 * ********************************************************* */
#define USE_SD


#ifdef USE_SD
#include "ff.h"
#include "ffconf.h"
#endif

/* PROTOS */
int inputline(char *buffer, int size);

/* ************************************************************
 * DEFINES
 * ********************************************************* */
#define SUCCESS 1
#define FAILURE 0

#define DCFG_DEVICE_ID  XPAR_XDCFG_0_DEVICE_ID

/* ************************************************************
 * Application specific (To be removed)
 * ********************************************************* */

#define CONTROL    (0x43c00000)
#define INPUT_REG  (0x43c00010)
#define OUTPUT_REG (0x43c00018)

#define CPU_BASEADDR XPAR_SCUGIC_CPU_BASEADDR
#define DIST_BASEADDR XPAR_SCUGIC_DIST_BASEADDR

#define DEFAULT_PRIORITY 0xa0a0a0a0UL
#define DEFAULT_TARGET 01010101UL

volatile u32 InterruptID = 0;
volatile u32 irqProcessed = 0;

XScuGic_Config *gic_config = NULL;
XScuGic gic;

/* AXI TIMER */
XTmrCtr_Config *tmr_config;
XTmrCtr tmr;


/* For response time experiments */
XTime time_start;
volatile XTime time_end;
volatile int had_reply = 0;


#define CLOCK_FREQ XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ
#define MSEC_PER_SEC 1000
#define USEC_PER_SEC 1000000
#define NSEC_PER_SEC 1000000000

void irqHandler(void *callbackRef) {
  //u32 BaseAddress;
  //u32 IntID;  /* interrupt id */

  //XScuGic_Disable(&gic, 61);
  Xil_ExceptionDisable();

  XTime_GetTime(&time_end);

  /* Clear the output GPIO pin */
  *(volatile u32*)0x41200008 = 0x0;
  Xil_Out32(0x41200008,0x0);
  Xil_DCacheFlushRange(0x41200008, 4);


  u32 r = XGpio_ReadReg(0x41200000, XGPIO_ISR_OFFSET);
  XGpio_WriteReg(0x41200000, XGPIO_ISR_OFFSET, r & XGPIO_IR_MASK );

  /* Disable interrupt in GPIO block */
  //u32 Register = XGpio_ReadReg(0x41200000, XGPIO_IER_OFFSET);
  //XGpio_WriteReg(0x41200000, XGPIO_IER_OFFSET,
  //      Register &  ~XGPIO_IR_MASK);

  //u32 IntID = XScuGic_ReadReg(XPAR_SCUGIC_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET) & XSCUGIC_ACK_INTID_MASK;

  //xil_printf("Entering irqHandler\n\rID: %d \n\r", IntID);
  //xil_printf("Running irqHandler\n\r");

  if (callbackRef == NULL) {
    xil_printf("CallbackRef is NULL, returning\n\r");
  }

  //BaseAddress =  (u32)callbackRef;

  //IntID = IntID & XSCUGIC_ACK_INTID_MASK;

  // xil_printf("Interrupt ID: %d\n\r", IntID);

  //if (XSCUGIC_MAX_NUM_INTR_INPUTS < IntID) {
  //  printf("ID > MAX_NUM_INTR_INPUTS \n\r");
  //  return;
 // }

  //InterruptID = IntID;
  irqProcessed++;
  had_reply = 1;

  //XScuGic_Enable(&gic, 61);
  Xil_ExceptionEnable();

}
/* response time measurement experiment */
int experiment1_cmd(int n, char **args) {



  u32 num_samples;

  u64 diff;
  u64 clock_ticks;

  if (n < 2 || n > 2) {
    xil_printf("Wrong number of arguments\n\r");
    return 0;
  }

  num_samples = atoi(args[1]);

  xil_printf("Starting experiment\n\r\n\r");

  printf("run_id, usec (cpu), num_ticks at 200MHz, usecs (axi timer) \n\r");
  for (int i = 0; i < num_samples; i ++) {

    /* wait for input pin to go low */
     while (Xil_In32(0x41200000) != 0) ;

    XTime_GetTime(&time_start);
    *(volatile unsigned int*)0x41200008 = 1; /* set output GPIO pin */
    //Xil_DCacheFlushRange(0x41200008, 4);


    while (!had_reply);

    diff = time_end - time_start;


    //printf("Cap1: %u \n\r", XTmrCtr_GetCaptureValue(&tmr,0));
    //printf("Cap2: %u \n\r", XTmrCtr_GetCaptureValue(&tmr,1));
    u32 tmr0 = XTmrCtr_GetCaptureValue(&tmr,0);
    u32 tmr1 = XTmrCtr_GetCaptureValue(&tmr,1);

    u32 tmr_diff = tmr1 - tmr0;



    float tmr_diff_s = (float)tmr_diff / 200000000.0;

    clock_ticks = diff * 2; /* performance counter only ticks once every two clock-ticks */

    //printf("Start: %llu\n\r",time_start);
    //printf("Reply: %llu\n\r",time_end);
    //printf("Waited for %llu ticks for a reply\n\r",clock_ticks);

    double s = (double)clock_ticks / (double)CLOCK_FREQ;

    //printf("seconds: %f\n\r",s);
    //printf("ms: %f\n\r",s*MSEC_PER_SEC);
    printf("%d, %f, %u, %f\n\r",i, s*USEC_PER_SEC, tmr_diff, tmr_diff_s*USEC_PER_SEC);
    //printf("ns: %f\n\r",s*NSEC_PER_SEC);

    /* prepare to run again */
    had_reply = 0;
  }

  return 1;
}


void init_app() {

  int status;

  //Xil_SetTlbAttributes(CONTROL, NORM_NONCACHE);
  xil_printf("Initializing app specific settings!\n\r");
  /* GIC DIST INIT */

  gic_config = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
  if (!gic_config) printf("ERROR looking up GIC config\n\r");


  // Initialise the GIC using the config information
  status = XScuGic_CfgInitialize(&gic, gic_config, gic_config->CpuBaseAddress);

  if (status != XST_SUCCESS) printf ("ERROR initializing GIC configuration\n\r");


  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
                      (Xil_ExceptionHandler)XScuGic_InterruptHandler, &gic);

  Xil_ExceptionEnable();

  status = XScuGic_Connect(&gic, 61,(Xil_ExceptionHandler)irqHandler,(void *)&gic);
  if (status != XST_SUCCESS) printf ("ERROR connecting handler\n\r");

  XScuGic_SetPriorityTriggerType(&gic,61,0, 0x3);


  XScuGic_Enable(&gic, 61);

  /* Setup the GPIO interrupt */
  XGpio_WriteReg(0x41200000, XGPIO_GIE_OFFSET,
        XGPIO_GIE_GINTR_ENABLE_MASK);

  u32 Register = XGpio_ReadReg(0x41200000, XGPIO_IER_OFFSET);
  XGpio_WriteReg(0x41200000, XGPIO_IER_OFFSET,
        Register | XGPIO_IR_MASK);

  //xil_printf(" GPIO INTR: %u\n\r"); XGpio_ReadReg(0x41200000, XGPIO_IER_OFFSET);

  /* Enable interrupt */
  // XScuGic WriteReg(DIST_BASEADDR, XSCUGIC_ENABLE_SET_OFFSET +())

  //u32 Mask = 0x00000001U << ( 28 % 32U);


  /* initiate AXI timer */

  tmr_config = XTmrCtr_LookupConfig(XPAR_AXI_TIMER_0_DEVICE_ID);

  XTmrCtr_CfgInitialize(&tmr, tmr_config, XPAR_AXI_TIMER_0_BASEADDR );

  if (XST_SUCCESS != XTmrCtr_Initialize(&tmr,XPAR_AXI_TIMER_0_DEVICE_ID )) {
    xil_printf("AXI Timer initialization failed!\n\r");
  } else {
    xil_printf("AXI Timer initialized!\n\r");
  }

  if (XST_SUCCESS != XTmrCtr_SelfTest(&tmr,0)) printf("FAILED SELFTEST 0\n\r");
  if (XST_SUCCESS != XTmrCtr_SelfTest(&tmr,1)) printf("FAILED_SELFTEST 1\n\r");


  XTmrCtr_SetOptions(&tmr,1, XTC_CAPTURE_MODE_OPTION);
  XTmrCtr_SetOptions(&tmr,0, XTC_CAPTURE_MODE_OPTION | XTC_ENABLE_ALL_OPTION);
  //XTmrCtr_SetOptions(&tmr,1, XTC_CAPTURE_MODE_OPTION | XTC_ENABLE_ALL_OPTION);
  XTmrCtr_SetResetValue(&tmr,0 ,0);
  XTmrCtr_SetResetValue(&tmr,1 ,0);

  //XTmrCtr_Start(&tmr,0);

  xil_printf("Verifying that timers are running");
  u32 tim0 = XTmrCtr_GetValue(&tmr,0 );
  u32 tim1 = XTmrCtr_GetValue(&tmr,1 );
  xil_printf("... ");
  if (XTmrCtr_GetValue(&tmr,0 ) != tim0 &&
      XTmrCtr_GetValue(&tmr,1 ) != tim1) {
    xil_printf("OK!\n\r");
  } else {
    xil_printf("ERROR: Timers are not running\n\r");
  }

  //xil_printf("Cap1: %u \n\r", XTmrCtr_GetCaptureValue(&tmr,0));
  //xil_printf("Cap2: %u \n\r", XTmrCtr_GetCaptureValue(&tmr,1));

  //XTmrCtr_Stop(&tmr, 0);
  //XTmrCtr_Stop(&tmr, 1);
  //XTmrCtr_Reset(&tmr, 0);
  //XTmrCtr_Reset(&tmr, 1);

  //printf("RESET t0: %d \n\r", XTmrCtr_GetValue(&tmr,0 ));
  //printf("RESET t1: %d \n\r", XTmrCtr_GetValue(&tmr,1 ));
  //printf("RESET t0: %d \n\r", XTmrCtr_GetValue(&tmr,0 ));
  //printf("RESET t1: %d \n\r", XTmrCtr_GetValue(&tmr,1 ));
}

/* ************************************************************
 * Globals
 * ********************************************************* */
int running = 1; /* Start out in running state */

XDcfg DcfgInstance; /* Device configuration "instance" */

#ifdef USE_SD
#define MAX_PATH 256
FATFS fat_fs;
int sd_ok = 0; /* assume not ok to begin with */
char pwd[MAX_PATH] = "";
#endif

#define BYTE_TYPE 0
#define INT_TYPE 1
#define UINT_TYPE 2
#define FLOAT_TYPE 3

const char *type_str[] = { "byte", "int", "uint", "float" };
const int  type_size[] = { 1, 4, 4, 4 };

#define MAX_ALLOCATED_ARRAYS 20
typedef struct {
  int available;
  char *data;
  int type;
  int size; /* in elements of type type */
} array;

array arrays[MAX_ALLOCATED_ARRAYS];

#define MAX_PROMPT_SIZE 256
char prompt[MAX_PROMPT_SIZE] = "> ";

const int max_tokens = 20; /* Maximum number of tokens*/

const char *logo =
    "    _____             _____ _       _ _ \n\r"
    "   |__   |_ _ ___ ___|   __| |_ ___| | |\n\r"
    "   |   __| | |   | . |__   |   | -_| | |\n\r"
    "   |_____|_  |_|_|_  |_____|_|_|___|_|_|\n\r"
    "         |___|     |_|                  \n\r";



const char *header = "\n\rZynqShell\n\rtyping \"help\" shows a list of applicable commands\n\r";
const char *tokdelim = "\n\t\r ";

const char *cmds[] = {"help"
                      ,"exit"
                      ,"q"
                      ,"mread"
                      ,"mwrite"
                      ,"show"
                      ,"loadArray"
                      ,"mkArray"
                      ,"cf"
                      ,"ci"
#ifdef USE_SD
                      ,"ls"
                      ,"sdLoad"
                      ,"sdStore"
#endif
                      ,"programFPGA"
                      ,"ex1"
                      };

const char *hlp_str =
  "----------------------------------------------------------------------\n\r"\
  "help [about] - (with no arguments) Displays this message.\n\r"\
  "      Valid abouts: Currently no valid abouts.\n\r"\
  "exit - Exits from ZynqShell\n\r"\
  "mread <type> <address> [num_elements] - \n\r"\
  "      Read data from memory location <address> and interpret it as\n\r"\
  "      type <type>.\n\r"\
  "      Valid types: int - 32bit signed integer.\n\r"\
  "                   uint - 32bit unsigned integer.\n\r"\
  "                   float - 32bit floating point.\n\r"\
  "mwrite <type> <address> <value>\n\r"\
  "show <what> - \n\r"\
  "     show information about <what>. \n\r"\
  "     Valid whats: arrays -  show information about allocated arrays.\n\r"\
  "                  array <id> - show array <id>.\n\r"\
  "loadArray <type> <num_elements> [ID] - Load elements into an array.\n\r"\
  "     Will expext <num_elements> lines of input containing\n\r"\
  "     data that is parseable as <type>.\n\r"\
  "mkArray <type> <num_elements> [ID] - Allocate an array.\n\r"\
  "sdLoad <filename> <array_id> - Load a file from sd card into array of given id.\n\r"\
  "sdStore <filename> <array_id> - Store array of given id into file.\n\r"\
  "programFPGA <array_id> - Program FPGA using data stored in array.\n\r"\
  "     Contents of array should be a valid FPGA configuration bitstream."\
  "cf - Cache flush.\n\r"\
  "ci - Cache invalidate.\n\r"\
  "----------------------------------------------------------------------\n\r";

/* ************************************************************
 * Array management helper functions
 * ********************************************************* */

int freeArray(int id) {
  if (id >= 0 && id < MAX_ALLOCATED_ARRAYS) {
    if (!arrays[id].available) {
      free(arrays[id].data);
      arrays[id].data = NULL;
      arrays[id].available = 1;
      arrays[id].size = 0;
      arrays[id].type = BYTE_TYPE;
    }
    return SUCCESS;
  }
  return FAILURE;
}

/* TODO: Some kind of success/failure checking, maybe? */
void freeArrays() {
  int i = 0;
  for (i = 0; i < MAX_ALLOCATED_ARRAYS; i ++) {
    freeArray(i);
  }
}


/* ************************************************************
 * Implementation of commands
 * ********************************************************* */

int exit_cmd(int n, char **args) {
  running = 0;
  return SUCCESS;
}

/* prints help message. Always returns success */
int help_cmd(int n, char **args) {

  if (n < 1 || n > 2) {
      xil_printf("Wrong number of arguments!\n\rUsage: help [about]\n\r");
  }
  if (n == 2) {
    xil_printf("Currently no valid abouts.\n\r");
  } else {
    xil_printf("%s", hlp_str);
  }
  return SUCCESS;
}

int mread_cmd(int n, char **args) {

  unsigned int address;
  unsigned int num_elts = 1; /* 1 element by default */
  int i = 0;

  if (n < 3 || n > 4) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage: mread <type> <address>\n\r");
    return FAILURE;
  }

  sscanf(args[2], "%x", &address);
  /* xil_printf("%x - %u\n\r", address, address); */

  if (n == 4) {
    num_elts = atoi(args[3]);
    if (num_elts < 1)
      return FAILURE;
  }

  Xil_DCacheFlushRange(address, num_elts * 4); /* TODO fix */


  for (i = 0; i < num_elts; i++) {

    if (strcmp(args[1], "int") == 0) {
      xil_printf("%d\n\r", *(int*) (address + i * sizeof(int)));
    } else if (strcmp(args[1], "uint") == 0) {
      xil_printf("%u\n\r",
          *(unsigned int*) (address + i * sizeof(unsigned int)));
    } else if (strcmp(args[1], "float") == 0) {
      char tmp[128];
      snprintf(tmp, 128, "%f", *(float*) (address + i * sizeof(float)));
      xil_printf("%s\n\r", tmp);
    } else if (strcmp(args[1], "byte") == 0) {
      xil_printf("%d\n\r", *(unsigned char*) (address + i));
    } else {
      xil_printf("Incorrect type specifier\n\r");
      return FAILURE;
    }
  }
  return SUCCESS;
}

int mwrite_cmd(int n, char **args) {

  unsigned int address;

  if (n < 4 || n > 4) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage: mwrite <type> <address> <value>\n\r");
    return FAILURE;
  }

  sscanf(args[2], "%x", &address);
  /* xil_printf("%x - %u\n\r", address, address); */

  if (strcmp(args[1], "int") == 0) {
    int val;
    if (strncmp(args[3],"0x", 2) == 0) { /* Inteprete as hex */
      sscanf(args[3],"%x", &val);
    } else {
      val = atoi(args[3]);
    }
    *(volatile int*) address = val;
    Xil_DCacheFlushRange(address, sizeof(int));

  } else if (strcmp(args[1], "uint") == 0) {
    unsigned int val;
    if (strncmp(args[3],"0x", 2) == 0) { /* Inteprete as hex */
      sscanf(args[3],"%x", &val);
    } else {
      val = atoi(args[3]);
    }
    *(volatile unsigned int*) address = val;
    Xil_DCacheFlushRange(address, sizeof(unsigned int));
  } else if (strcmp(args[1], "float") == 0) {
    *(volatile float*) address = atof(args[3]);
    Xil_DCacheFlushRange(address, sizeof(float));
  } else {
    xil_printf("Incorrect type specifier\n\r");
    return FAILURE;
  }

  return SUCCESS;
}

void printByte(char *ptr, int i) {
  xil_printf("%d", ((char*)ptr)[i]);
}
void printInt(char *ptr, int i) {
  xil_printf("%d", ((int*)ptr)[i]);
}
void printUInt(char *ptr, int i) {
  xil_printf("%u", ((unsigned int *)ptr)[i]);
}
void printFloat(char *ptr, int i) {
  char buffer[256];
  snprintf(buffer,256,"%f",((float*)ptr)[i]);
  xil_printf("%s", buffer);
}

int printArray(int id) {
  int i = 0;
  int bytes = arrays[id].size;

  void (*printer)(char *, int) = NULL;

  switch (arrays[id].type) {
  case INT_TYPE:
    printer = printInt;
    bytes *= sizeof(int);
    break;
  case FLOAT_TYPE:
    printer = printFloat;
    bytes = sizeof(float);
    break;
  case BYTE_TYPE:
    printer = printByte;
    break;
  case UINT_TYPE:
    printer = printUInt;
    bytes *= sizeof(unsigned int);
    break;
  default:
    xil_printf("Unsupported type\n\r");
    return FAILURE;
  }

  Xil_DCacheInvalidateRange((unsigned int)arrays[id].data, bytes);
  for (i = 0; i < arrays[id].size; i ++) {
    printer(arrays[id].data, i);
    xil_printf("\n\r");
  }
  return SUCCESS;
}

int show_cmd(int n, char **args) {

  int i;

  if (n < 2 ) {
    xil_printf("Wrong number of arguments!\n\rUsage: show <what>\n\r");
    return FAILURE;
  }

  if (strcmp(args[1], "arrays") == 0) {
    xil_printf("ID\t Status\t Addr\t Type\t Size\n\r");
    for (i = 0; i < MAX_ALLOCATED_ARRAYS; i++) {
      xil_printf("%d\t %s\t %x\t %s\t %d\n\r", i,
          arrays[i].available ? "Free" : "Used", (unsigned int) arrays[i].data,
          type_str[arrays[i].type], arrays[i].size);
    }
  } else if (strcmp (args[1], "array") == 0) {

    if (n < 3) {
      xil_printf("Requires an ArrayID argument!\n\r");
      return FAILURE;
    }
    int id = atoi(args[2]);
    if (arrays[id].available) {
      xil_printf("Available\n\r",id);
    } else {
      printArray(id);
    }

  } else {
    xil_printf("No information available on %s\n\r", args[1]);
  }
  return SUCCESS;

}

int loadArray_cmd(int n, char **args) {

  int num = 0;
  int use_id = -1; // initialise to -1 for check if available was found
  int i = 0;
  char buffer[256];
  int bytes = 0;

  if (n < 3 || n > 4) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage: loadArray <type> <num_elements> [ID]\n\r");
    return FAILURE;
  }

  num = atoi(args[2]);
  if (n == 4) {
    use_id = atoi(args[3]);

    if (use_id < 0 || use_id > MAX_ALLOCATED_ARRAYS) {
      xil_printf("Incorrect array id!\n\r");
      return FAILURE;
    }
  } else { /* find free array slot */
    for (i = 0; i < MAX_ALLOCATED_ARRAYS; i++) {
      if (arrays[i].available) {
        use_id = i;
        break;
      }
    }
  }
  if (use_id == -1) {
    xil_printf("No available array slot\n\r");
    return FAILURE;
  }

  /* If we arrive here, we have selected a slot */

  if (strcmp(args[1], "int") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *)malloc(num * sizeof(int));
    arrays[use_id].type = INT_TYPE;
    for (i = 0; i < num; i++) {
      inputline(buffer, 256);
      xil_printf("\n\r");
      int val = atoi(buffer);
      ((int*) arrays[use_id].data)[i] = val;
      }
    bytes = num * sizeof(int);
  } else if (strcmp(args[1], "uint") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *)malloc(num * sizeof(unsigned int));
    arrays[use_id].type = UINT_TYPE;
    for (i = 0; i < num; i ++)  {
      inputline(buffer, 256);
      xil_printf("\n\r");
      unsigned int val = atoi(buffer);
      ((unsigned int*)arrays[use_id].data)[i] = val;
    }
    bytes = num * sizeof(unsigned int);
  } else if (strcmp(args[1], "float") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *)malloc(num * sizeof(float));
    arrays[use_id].type = FLOAT_TYPE;
    for (i = 0; i < num; i ++) {
      inputline(buffer,256);
      xil_printf("\n\r");
      float val = atof(buffer);
      ((float*)arrays[use_id].data)[i] = val;
    }
    bytes = num * sizeof(float);
  } else {
    /*
     * Unsupported type
     */
    //arrays[use_id].available = 1;
    //arrays[use_id].size = 0;
    xil_printf("type %s not yet supported\n\r", args[1]);
    return FAILURE;
  }
  dsb();
  //xil_printf("flushing %d bytes at address %x\n\r", bytes, (unsigned int)arrays[use_id].data);
  xil_printf("flushing cache\n\r");
  Xil_DCacheFlushRange((unsigned int)arrays[use_id].data, bytes);
  //Xil_DCacheFlush();

  return SUCCESS;
}

int mkArray_cmd(int n, char **args) {
  int i = 0;
  int use_id = -1;
  int num = 0;

  if (n < 3 || n > 4) {
    xil_printf(
            "Wrong number of arguments!\n\rUsage: mkArray <type> <num_elements> [ID]\n\r");
        return FAILURE;
  }

  num = atoi(args[2]);

  if (n == 4) {
    use_id = atoi(args[3]);

    if (use_id < 0 || use_id > MAX_ALLOCATED_ARRAYS) {
      xil_printf("Incorrect array id!\n\r");
      return FAILURE;
    }
    /* free the array */
    freeArray(use_id);

  } else { /* find free array slot */
    for (i = 0; i < MAX_ALLOCATED_ARRAYS; i++) {
      if (arrays[i].available) {
        use_id = i;
        break;
      }
    }
  }
  if (use_id == -1) {
    xil_printf("No available array slot\n\r");
    return FAILURE;
  }

  if (strcmp(args[1], "int") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *) malloc(num * sizeof(int));
    memset(arrays[use_id].data,0,num * sizeof(int));
    arrays[use_id].type = INT_TYPE;
  } else if (strcmp(args[1], "uint") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *) malloc(num * sizeof(unsigned int));
    memset(arrays[use_id].data,0,num * sizeof(unsigned int));
    arrays[use_id].type = UINT_TYPE;
  } else if (strcmp(args[1], "float") == 0) {
    if (!arrays[use_id].available) { // if selected is in use
      freeArray(use_id);
    }
    arrays[use_id].size = num;
    arrays[use_id].available = 0;
    arrays[use_id].data = (char *) malloc(num * sizeof(float));
    memset(arrays[use_id].data,0,num * sizeof(float));
    arrays[use_id].type = FLOAT_TYPE;
  } else {
    /*
     * Unsupported type
     */
    xil_printf("type %s not yet supported\n\r", args[1]);
    return FAILURE;
  }
  return SUCCESS;
}

int cf_cmd(int n, char **args) {
  Xil_DCacheFlush();
  return SUCCESS;
}

int ci_cmd(int n, char **args) {
  Xil_DCacheInvalidate();
  return SUCCESS;
}

/* ************************************************************
 * FatFS, Files, Directories
 * ********************************************************* */
#ifdef USE_SD

FRESULT ls(char* path){
  FRESULT res;
  DIR dir;
  static FILINFO fno;

  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      xil_printf("%s/%s\n\r", path, fno.fname);
    }
    f_closedir(&dir);
  }
  return res;
}

int ls_cmd(int n, char **args) {
  FRESULT res;
  /* ignores all arguments, if any */
  res = ls(pwd);
  if (res != FR_OK) return FAILURE;

  return SUCCESS;
}

int load_raw(char *path, int array_id) {

  FIL fp;
  int r;
  int size;

  r = f_open(&fp, path, FA_READ);
  if ( r != FR_OK) {
    xil_printf("Error opening file\n\r");
    return FAILURE;
  } else {

    size = file_size(&fp);

    /* check if array is free or used */
    if (!arrays[array_id].available) {
      free(arrays[array_id].data);
    }

    /* allocate storage for data from file */
    if ((arrays[array_id].data = (char*)malloc(size))) {
      arrays[array_id].available = 0;
      arrays[array_id].type = BYTE_TYPE;
      arrays[array_id].size = size;
    } else {
      xil_printf("Error allocating memory for file contents\n\r");
      return FAILURE;
    }

    unsigned int rd = 0;
    f_read(&fp, arrays[array_id].data, size, &rd);
    f_close(&fp);
  }

  return SUCCESS;
}

int sd_load_raw_cmd(int n, char **args) {

  int array_id = 0;
  char path[MAX_PATH];

  if (n < 3 || n > 3) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage:  sdLoad <filename> <Array ID>\n\r");
    return FAILURE;
  }

  array_id = atoi(args[2]);

  strncpy(path,pwd,MAX_PATH);
  strncat(path,args[1],MAX_PATH - strlen(path));

  xil_printf("Loading file: %s\n\r", path);
  load_raw(path,array_id);
  return SUCCESS;
}


int store_raw(char *path, int array_id) {

  FIL fp;
  int r;
  int size;

  if(arrays[array_id].available) {
    xil_printf("Error: input array is not in use\n\r");
    return FAILURE;
  }

  size = arrays[array_id].size * (type_size[arrays[array_id].type]);

  r = f_open(&fp, path, FA_WRITE | FA_CREATE_NEW);
  if ( r != FR_OK) {
    xil_printf("Error: %d\n\r", r);
    return FAILURE;
  } else {

    unsigned int wrt = 0;
    f_write(&fp, arrays[array_id].data, size, &wrt);
    f_close(&fp);
    printf("%d Bytes written to file\n\r",wrt);
  }
  return SUCCESS;
}

/* sdStore <filename> <array_id>*/
int sd_store_raw_cmd(int n, char **args) {

  int array_id = 0;
  char path[MAX_PATH];

  if (n < 3 || n > 3) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage:  sdStore <filename> <Array ID>\n\r");
    return FAILURE;
  }

  array_id = atoi(args[2]);
  if (array_id < 0 || array_id > MAX_ALLOCATED_ARRAYS) {
    xil_printf("Error: Faulty array ID\n\r");
    return FAILURE;
  }

  strncpy(path,pwd,MAX_PATH);
  strncat(path,args[1],MAX_PATH - strlen(path));

  xil_printf("Storing to file: %s\n\r", path);

  store_raw(path,array_id);

  return SUCCESS;
}

#endif

/* ************************************************************
 * PROGRAM FPGA WITH BITSTREAM
 * ********************************************************* */
int program_bitstream(XDcfg *Instance, u32 StartAddress, u32 WordLength)
{
  int Status;
  volatile u32 IntrStsReg = 0;

  // Clear DMA and PCAP Done Interrupts
  XDcfg_IntrClear(Instance, (XDCFG_IXR_DMA_DONE_MASK | XDCFG_IXR_D_P_DONE_MASK));

  // Transfer bitstream from DDR into fabric in non secure mode
  Status = XDcfg_Transfer(Instance, (u32 *) StartAddress, WordLength, (u32 *) XDCFG_DMA_INVALID_ADDRESS, 0, XDCFG_NON_SECURE_PCAP_WRITE);
  if (Status != XST_SUCCESS)
    return Status;

  // Poll DMA Done Interrupt
  while ((IntrStsReg & XDCFG_IXR_DMA_DONE_MASK) != XDCFG_IXR_DMA_DONE_MASK)
    IntrStsReg = XDcfg_IntrGetStatus(Instance);

  // Poll PCAP Done Interrupt
  while ((IntrStsReg & XDCFG_IXR_D_P_DONE_MASK) != XDCFG_IXR_D_P_DONE_MASK)
    IntrStsReg = XDcfg_IntrGetStatus(Instance);

  return XST_SUCCESS;
}

int programFPGA_cmd(int n, char **args) {

  int array_id;
  XDcfg_Config *ConfigPtr;
  int status;

  if (n < 2 || n > 2) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage:  programFPGA <Array ID>\n\r");
    return FAILURE;
  }

  array_id = atoi(args[1]);

  if (array_id < 0 || array_id >= MAX_ALLOCATED_ARRAYS) {
    xil_printf("Incorrect Array ID\n\r");
    return FAILURE;
  }

  if (arrays[array_id].available) {
    xil_printf("Array is empty\n\r");
    return FAILURE;
  }

  /* Maybe can be moved to some, run once, init procedure */
  ConfigPtr = XDcfg_LookupConfig(DCFG_DEVICE_ID);

  status = XDcfg_CfgInitialize(&DcfgInstance, ConfigPtr,
            ConfigPtr->BaseAddr);
  if (status != XST_SUCCESS) {
    xil_printf("Failed to initialize DevCFG driver\n\r");
    return FAILURE;
  }
  status = XDcfg_SelfTest(&DcfgInstance);
  if (status != XST_SUCCESS) {
    xil_printf("Failed DevCFG self test\n\r");
    return FAILURE;
  }

  status = program_bitstream(&DcfgInstance,
                             (u32)arrays[array_id].data,
                             arrays[array_id].size >> 2);

  if (status != XST_SUCCESS) {
    xil_printf("Failed to program FPGA\n\r");
    return FAILURE;
  }
  xil_printf("OK!\n\r");
  return SUCCESS;
}


/* ************************************************************
 * Command function array
 * ********************************************************* */

/* Array of command functions */
int (*cmd_func[])(int, char **) = {
  &help_cmd
  ,&exit_cmd
  ,&exit_cmd
  ,&mread_cmd
  ,&mwrite_cmd
  ,&show_cmd
  ,&loadArray_cmd
  ,&mkArray_cmd
  ,&cf_cmd
  ,&ci_cmd
#ifdef USE_SD
  ,&ls_cmd
  ,&sd_load_raw_cmd
  ,&sd_store_raw_cmd
#endif
  ,&programFPGA_cmd
  ,experiment1_cmd
};


/* ************************************************************
 * Code
 * ********************************************************* */

int tokenize(char *cmd_str, char ***tokens) {
  int num_tokens = 0;
  char *token;

  token = strtok(cmd_str, tokdelim);
  while (token) {
    /* Abort if out of space */
    if (num_tokens == max_tokens) {
      xil_printf("\n\rToo many arguments!\n\r");
      return 0;
    }
    (*tokens)[num_tokens++] = token;

    token = strtok(NULL, tokdelim);
  }
  return num_tokens;
}


int dispatch(int num_toks, char **tokens) {
  int i = 0;
  int n_cmds = sizeof(cmds) / sizeof(char *);

  if (!num_toks)
    return 1;

  for (i = 0; i < n_cmds; i++) {
    if (strcmp(tokens[0], cmds[i]) == 0) {
      return (*cmd_func[i])(num_toks, tokens);
    }
  }

  xil_printf("%s: command not found\n\r", tokens[0]);
  return 1;
}

int inputline(char *buffer, int size) {
  int n = 0;
  char c;

  for (n = 0; n < size - 1; n++) {

    c = inbyte();
    switch (c) {
    case 127: /* fall through to below */
    case '\b': /* backspace character received */
      if (n > 0)
        n--;
      buffer[n] = 0;
      outbyte('\b'); /* output backspace character */
      n--; /* set up next iteration to deal with preceding char location */
      break;
    case '\n': /* fall through to \r */
    case '\r':
      buffer[n] = 0;
      return n;
    default:
      if (isprint(c)) { /* ignore non-printable characters */
        outbyte(c);
        buffer[n] = c;
      } else {
        n -= 1;
      }
      break;
    }
  }
  buffer[size - 1] = 0;
  return 0; // Filled up buffer without reading a linebreak
}

/* ************************************************************
 * Main
 * ********************************************************* */

int main()
{
  char *cmd_buffer;
  size_t  cmd_buffer_size = 512;
  int result;

  char **tokens;
  int n = 0;
  int status = 0;
  int i = 0;

  init_platform();
  printf("\n\r\n\r");

  init_app();

  /* Initialisation */
  cmd_buffer = malloc(cmd_buffer_size * sizeof(char));
  tokens = malloc(max_tokens * sizeof(char*));

#ifdef USE_SD
  /* Initialise file system */
  result = f_mount(&fat_fs,"0:", 0);
  if (result != FR_OK) {
    xil_printf("Error mounting file system!\n\r");
  } else {
    xil_printf("File system successfully mounted!\n\r");
    sd_ok = 1;
  }
#endif

  /* Initialise array storage */
  for (i = 0; i < MAX_ALLOCATED_ARRAYS; i ++) {
    arrays[i].available = 1;
    arrays[i].data = NULL;
    arrays[i].type = BYTE_TYPE;
    arrays[i].size = 0;
  }

  /* Print welcoming header */
  xil_printf("%s", logo);
  xil_printf("%s", header);

  /* The command parsing and executing loop */
  while(running) {
    xil_printf("[%d]%s ", irqProcessed, prompt);
    inputline(cmd_buffer, cmd_buffer_size);

    xil_printf("\n\r");

    n = tokenize(cmd_buffer, &tokens);

    status = dispatch(n, tokens);

    if (!status) {
      xil_printf("Error executing command!\n\r");
    }
  }

  freeArrays();
  free(tokens);
  free(cmd_buffer);

  cleanup_platform();
  return 0;
}
