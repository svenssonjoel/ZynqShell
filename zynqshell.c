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

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

/* PROTOS */
int inputline(char *buffer, int size);


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
                      "loadArray"
                      };

const char *hlp_str =
  "----------------------------------------------------------------------\n\r"\
  "help [about] - (with no arguments) Displays this message\n\r"\
  "      Valid abouts: extension - displays extension help\n\r"\
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
  "loadArray <type> <num_elements> [ID] - Load elements into an array\n\r"\
  "     Will expext <num_elements> lines of input containing\n\r"\
  "     data that is parseable as <type>\n\r"\
  "----------------------------------------------------------------------\n\r";

/* ************************************************************
 * Extensions Interface
 * ********************************************************* */
char* (*extension_help)() = NULL; /* extension should return help string */
int (*extension_dispatch)(int, char **) = NULL; /*extension command dispatcher */

void setExtensionHelp(char* (*ptr)()) {
  extension_help = ptr;
}

void setExtensionDispatch(int (*ptr)(int, char**)) {
  extension_dispatch = ptr;
}



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
    if (strcmp(args[1], "extension") == 0) {
      if (extension_help) {
        xil_printf("%s", extension_help());
      } else {
        xil_printf("No extension help available\n\r");
      }
    }
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
    *(int*) address = atoi(args[3]);
  } else if (strcmp(args[1], "uint") == 0) {
    *(unsigned int*) address = atoi(args[3]);
  } else if (strcmp(args[1], "float") == 0) {
    *(float*) address = atof(args[3]);
  } else {
    xil_printf("Incorrect type specifier\n\r");
  }

  return 1;
}

int show_cmd(int n, char **args) {

  int i;

  if (n < 2 || n > 2) {
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
    /* free the array */
    freeArray(use_id);
/*
    if (!arrays[use_id].available) {
      free(arrays[use_id].data);
      arrays[use_id].available = 1;
      arrays[use_id].data = NULL;
      arrays[use_id].size = 0;
      arrays[use_id].type = BYTE_TYPE;
    }
*/
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

  /* If we arrive here, we have a slot */
  arrays[use_id].size = num;
  arrays[use_id].available = 0;

  if (strcmp(args[1], "int") == 0) {
    arrays[use_id].data = (char *) malloc(num * sizeof(int));
    arrays[use_id].type = INT_TYPE;
    for (i = 0; i < num; i++) {
      inputline(buffer, 256);
      xil_printf("\n\r");
      xil_printf("buffer = %s\n\r", buffer);
      int val = atoi(buffer);
      xil_printf("atoi = %d\n\r", val);
      ((int*) arrays[use_id].data)[i] = val;
    }

  } else {
    xil_printf("type %s not yet supported\n\r", args[1]);
    return 0;
  }
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
  &loadArray_cmd
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

  if(extension_dispatch) {
    extension_dispatch(num_toks, tokens);
    return 1;
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

  const int scratch_size = 512;
  char *scratch;

  char **tokens;
  int n = 0;
  int status = 0;
  int i = 0;

  init_platform();

  /* Initialisation */
  cmd_buffer = malloc(cmd_buffer_size * sizeof(char));
  scratch = malloc(scratch_size * sizeof(char));
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
  xil_printf("Scratch memory: %x\n\r", (unsigned int)scratch);

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
