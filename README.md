# ZynqShell
Shell like program to run on the, bare-metal, Zynq while experimenting. The idea is that it will provide an easy path to 
upload data, start computations on the FPGA and inspect results.

# Functionality
1. Create arrays.
2. Show contents of arrays.
3. Load data into an array over the serial connection to the shell running on the Zynq. Each line of text transmitted will be interpreted by the shell as a given type. I like to use screen as terminal that has the capability to paste in larger amounts of data.
4. Read/Write to arbitrary memory locations while interpreting what is read/written as a C-type (int, unsigned int, float).
5. Load and store data from/to files on SD card.

# TODO
- Figure out more exactly where and when to flush caches and what ranges (useful when the next entity to read that value is the FPGA). 
 
# Howto
I currently build ZynqShell by creating a "hello world" application in Vivado SDK and replace the helloworld.c file there with 
the zynqshell.c file (ln -s to the file in the repo clone). 

# ZynqBerry SD configuration in Vivado

![sdconf](/pictures/zynqberry_sd_config.png)
