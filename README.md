# ZynqShell (work in progress)
Shell like program to run on the Zynq while experimenting. The idea is that it will provide an easy path to 
upload data, start computations on the FPGA and inspect results. I suspect there is a fair bit of a reusable core of this but
that for specific experiments, tweaks will be needed. 

# Howto
I currently build this by creating a "hello world" application in Vivado SDK and replace the helloworld.c file there with 
the zynqshell.c file (ln -s to the file in the repo clone). 
