# ZynqShell (work in progress)
Shell like program to run on the Zynq while experimenting. The idea is that it will provide an easy path to 
upload data, start computations on the FPGA and inspect results. I suspect there is a fair bit of a reusable core of this but
that for specific experiments, tweaks will be needed.

#Functionality
1. Create arrays
2. Show contents of arrays
3. Load data into array over the serial connection to the shell. I like to use "screen" as terminal.
4. Read/Write to arbitrary memory locations while interpreting what is there as a C-type (int, unsigned int, float)



# Howto
I currently build this by creating a "hello world" application in Vivado SDK and replace the helloworld.c file there with 
the zynqshell.c file (ln -s to the file in the repo clone). 
