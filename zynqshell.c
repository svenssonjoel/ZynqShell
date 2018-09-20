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

/* ************************************************************
 * Globals
 * ********************************************************* */
int running = 1; /* Start out in running state */

#define MAX_PROMPT_SIZE 256
char prompt[MAX_PROMPT_SIZE] = "> ";

const int max_tokens = 20; /* Maximum number of tokens*/

const char *header = "ZynqShell\n\rtyping \"help\" shows a list of applicable commands\n\r";
const char *tokdelim = "\n\t\r ";

const char *cmds[] = {"help",
		     	 	  "exit",
					  "q",
					  "mread",
					  "mwrite"
		     	 	  };

const char *hlp_str =
  "----------------------------------------------------------------------\n\r"\
  "help - Displays this message\n\r"\
  "exit - Exits from ZynqShell\n\r"\
  "mread <type> <address> - \n\r"\
  "      Read data from memory location <address> and interpret it as\n\r"\
  "      type <type>.\n\r"\
  "      Valid types: int - 32bit signed integer\n\r"\
  "                   uint - 32bit unsigned integer\n\r"\
  "                   float - 32bit floating point\n\r"\
  "mwrite <type> <address> <value>\n\r"\
  "----------------------------------------------------------------------\n\r";


/* ************************************************************
 * Implementation of commands
 * ********************************************************* */

int exit_cmd(int n, char **args) {
	running = 0;
	return 1;
}

int help_cmd(int n, char **args) {
    xil_printf("%s", hlp_str);
    return 1;
}

int mread_cmd(int n, char **args) {

	unsigned int address;

	if (n < 3 || n > 3) {
		printf("Wrong number of arguments!\n\rUsage: mread <type> <address>\n\r");
	    return 0;
	}

	sscanf(args[2],"%x", &address);
	/* xil_printf("%x - %u\n\r", address, address); */

	if (strcmp(args[1], "int") == 0) {
		xil_printf("%d\n\r", *(int*)address);
	} else if (strcmp(args[1], "uint") == 0) {
		xil_printf("%u\n\r", *(unsigned int*)address);
	}

	return 1;
}

int mwrite_cmd(int n, char **args) {

	unsigned int address;

	if (n < 4 || n > 4) {
		xil_printf("Wrong number of arguments!\n\rUsage: mwrite <type> <address> <value>\n\r");
	    return 0;
	}

	sscanf(args[2],"%x", &address);
	/* xil_printf("%x - %u\n\r", address, address); */

	if (strcmp(args[1], "int") == 0) {
		*(int*)address = atoi(args[3]);
	} else if (strcmp(args[1], "uint") == 0) {
		*(unsigned int*)address = atoi(args[3]);
	}
	return 1;
}

/* Array of command functions */
int (*cmd_func[]) (int, char **) = {
		&help_cmd,
		&exit_cmd,
		&exit_cmd,
		&mread_cmd,
		&mwrite_cmd
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

  if (!num_toks) return 1;

  for ( i = 0; i < n_cmds; i ++) {
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

	for (n = 0; n < size-1; n ++) {

		c = inbyte();
		switch (c) {
		case 8: /* backspace character received */
			if ( n > 0 ) n--;
			buffer[n] = 0;
			outbyte('\b'); /* output backspace character */
		    n--; /* set up next iteration to deal with preceding char location */
			break;
		case '\n':  /* fall through to \r */
		case '\r':
			buffer[n] = 0;
			return n;
		default:
			outbyte(c);
			buffer[n] = c;
			break;
		}
	}
	buffer[size-1] = 0;
	return 0; // Filled up buffer without reading a linebreak
}

/* ************************************************************
 * Screen help:
 *
 *  CTRL+h : sends backspace character
 *
 * ********************************************************* */

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

    init_platform();

    /* Initialisation */
    cmd_buffer = malloc(cmd_buffer_size * sizeof(char));
    scratch = malloc(scratch_size * sizeof(char));
    tokens = malloc(max_tokens * sizeof(char*));

    /* Print the welcome text */
    xil_printf("%s", header);
    xil_printf("Scratch memory: %x", (unsigned int)scratch);

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

    free(tokens);
    free(cmd_buffer);

    cleanup_platform();
    return 0;
}
