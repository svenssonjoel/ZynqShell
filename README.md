# ZynqShell (work in progress)
Shell like program to run on the, bare-metal, Zynq while experimenting. The idea is that it will provide an easy path to 
upload data, start computations on the FPGA and inspect results. I suspect there is a fair bit of a reusable core of this but
that for specific experiments, tweaks will be needed.

# Functionality
1. Create arrays.
2. Show contents of arrays.
3. Load data into an array over the serial connection to the shell running on the Zynq. Each line of text transmitted will be interpreted by the shell as a given type. I like to use screen as terminal that has the capability to paste in larger amounts of data.
4. Read/Write to arbitrary memory locations while interpreting what is read/written as a C-type (int, unsigned int, float).
5. Load raw data from files on SD card.

# Planned functionality
1. Store files to FAT formatted SD card.  

# Howto
I currently build ZynqShell by creating a "hello world" application in Vivado SDK and replace the helloworld.c file there with 
the zynqshell.c file (ln -s to the file in the repo clone). 
