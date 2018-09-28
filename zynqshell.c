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

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

/* PROTOS */
int inputline(char *buffer, int size);

/* ************************************************************
 * Application specific
 * ********************************************************* */

#define CONTROL    (0x43c00000)
#define INPUT_REG  (0x43c00008)
#define OUTPUT_REG (0x43c00018)

void init_app() {
  Xil_SetTlbAttributes(CONTROL, 0x10c06);
}


/* ************************************************************
 * Globals
 * ********************************************************* */
int running = 1; /* Start out in running state */

#define BYTE_TYPE 0
#define INT_TYPE 1
#define UINT_TYPE 2
#define FLOAT_TYPE 3

const char *type_str[] = { "byte", "int", "uint", "float" };

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

const char *header = "ZynqShell\n\rtyping \"help\" shows a list of applicable commands\n\r";
const char *tokdelim = "\n\t\r ";

const char *cmds[] = {"help",
                      "exit",
                      "q",
                      "mread",
                      "mwrite",
                      "show",
                      "loadArray",
                      "mkArray",
                      "cf",
                      "ci"
                      };

const char *hlp_str =
  "----------------------------------------------------------------------\n\r"\
  "help [about] - (with no arguments) Displays this message\n\r"\
  "      Valid abouts: Currently no valid abouts\n\r"\
  "exit - Exits from ZynqShell\n\r"\
  "mread <type> <address> [num_elements] - \n\r"\
  "      Read data from memory location <address> and interpret it as\n\r"\
  "      type <type>.\n\r"\
  "      Valid types: int - 32bit signed integer\n\r"\
  "                   uint - 32bit unsigned integer\n\r"\
  "                   float - 32bit floating point\n\r"\
  "mwrite <type> <address> <value>\n\r"\
  "show <what> - \n\r"\
  "     show information about <what>. \n\r"\
  "     Valid whats: arrays -  show information about allocated arrays\n\r"\
  "                  array <id> - show array <id>\n\r"\
  "loadArray <type> <num_elements> [ID] - Load elements into an array\n\r"\
  "     Will expext <num_elements> lines of input containing\n\r"\
  "     data that is parseable as <type>\n\r"\
  "mkArray <type> <num_elements> [ID] - allocate an array\n\r"\
  "cf - cache flush\n\r"\
  "ci - cache invalidate\n\r"\
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
    return 1;
  }
  return 0;
}

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
  return 1;
}

int help_cmd(int n, char **args) {

  if (n < 1 || n > 2) {
      xil_printf("Wrong number of arguments!\n\rUsage: help [about]\n\r");
  }
  if (n == 2) {
    xil_printf("Currently no valid abouts.\n\r");
  } else {
    xil_printf("%s", hlp_str);
  }
  return 1;
}

int mread_cmd(int n, char **args) {

  unsigned int address;
  unsigned int num_elts = 1; /* 1 element by default */
  int i = 0;

  if (n < 3 || n > 4) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage: mread <type> <address>\n\r");
    return 0;
  }

  sscanf(args[2], "%x", &address);
  /* xil_printf("%x - %u\n\r", address, address); */

  if (n == 4) {
    num_elts = atoi(args[3]);
    if (num_elts < 1)
      return 0;
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
    } else {
      xil_printf("Incorrect type specifier\n\r");
    }
  }
  return 1;
}

int mwrite_cmd(int n, char **args) {

  unsigned int address;

  if (n < 4 || n > 4) {
    xil_printf(
        "Wrong number of arguments!\n\rUsage: mwrite <type> <address> <value>\n\r");
    return 0;
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
    *(int*) address = val;
    Xil_DCacheFlushRange(address, sizeof(int));

  } else if (strcmp(args[1], "uint") == 0) {
    unsigned int val;
    if (strncmp(args[3],"0x", 2) == 0) { /* Inteprete as hex */
      sscanf(args[3],"%x", &val);
    } else {
      val = atoi(args[3]);
    }
    *(unsigned int*) address = val;
    Xil_DCacheFlushRange(address, sizeof(unsigned int));
  } else if (strcmp(args[1], "float") == 0) {
    *(float*) address = atof(args[3]);
    Xil_DCacheFlushRange(address, sizeof(float));
  } else {
    xil_printf("Incorrect type specifier\n\r");
  }

  return 1;
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
    return 0;
  }
  Xil_DCacheFlushRange((unsigned int)arrays[id].data, bytes);
  for (i = 0; i < arrays[id].size; i ++) {
    printer(arrays[id].data, i);
    xil_printf("\n\r");
  }
  return 1;
}

int show_cmd(int n, char **args) {

  int i;

  if (n < 2 ) {
    xil_printf("Wrong number of arguments!\n\rUsage: show <what>\n\r");
    return 0;
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
      return 0;
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
  return 1;

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
    return 0;
  }

  num = atoi(args[2]);
  if (n == 4) {
    use_id = atoi(args[3]);

    if (use_id < 0 || use_id > MAX_ALLOCATED_ARRAYS) {
      xil_printf("Incorrect array id!\n\r");
      return 0;
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
    return 0;
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
    return 0;
  }
  dsb();
  //xil_printf("flushing %d bytes at address %x\n\r", bytes, (unsigned int)arrays[use_id].data);
  xil_printf("flushing cache\n\r");
  //Xil_DCacheFlushRange((unsigned int)arrays[use_id].data, bytes);
  Xil_DCacheFlush();

  return 1;
}

int mkArray_cmd(int n, char **args) {
  int i = 0;
  int use_id = -1;
  int num = 0;

  if (n < 3 || n > 4) {
    xil_printf(
            "Wrong number of arguments!\n\rUsage: mkArray <type> <num_elements> [ID]\n\r");
        return 0;
  }

  num = atoi(args[2]);

  if (n == 4) {
    use_id = atoi(args[3]);

    if (use_id < 0 || use_id > MAX_ALLOCATED_ARRAYS) {
      xil_printf("Incorrect array id!\n\r");
      return 0;
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
    return 0;
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
    return 0;
  }
  return 1;
}

int cf_cmd(int n, char **args) {
  Xil_DCacheFlush();
  return 1;
}

int ci_cmd(int n, char **args) {
  Xil_DCacheInvalidate();
  return 1;
}

/* Array of command functions */
int (*cmd_func[])(int, char **) = {
  &help_cmd,
  &exit_cmd,
  &exit_cmd,
  &mread_cmd,
  &mwrite_cmd,
  &show_cmd,
  &loadArray_cmd,
  &mkArray_cmd,
  &cf_cmd,
  &ci_cmd
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

  char **tokens;
  int n = 0;
  int status = 0;
  int i = 0;

  init_platform();

  init_app();

  /* Initialisation */
  cmd_buffer = malloc(cmd_buffer_size * sizeof(char));
  tokens = malloc(max_tokens * sizeof(char*));

  /* Initialise array storage */
  for (i = 0; i < MAX_ALLOCATED_ARRAYS; i ++) {
    arrays[i].available = 1;
    arrays[i].data = NULL;
    arrays[i].type = BYTE_TYPE;
    arrays[i].size = 0;
  }

  /* Print the welcome text */
  xil_printf("%s", header);

  /* The command parsing and executing loop */
  while(running) {
    xil_printf("%s ", prompt);
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
