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
- Load FPGA configuration "bin files" and change the FPGA configuration at runtime (using devcfg). 
 
# Howto Build
I currently build ZynqShell by creating a "hello world" application in Vivado SDK and replace the helloworld.c file there with 
the zynqshell.c file (ln -s to the file in the repo clone). 

# ZynqBerry configuration in Vivado to enable SD card

Note that this is probably different for other boards, so consult the datasheets.

![sdconf](/pictures/zynqberry_sd_config.png)

# Screen tips

Example: "CTRL+a :" means "CTRL" and "a" together, followed by an ":". This particular command brings up a prompt where
additional commands can be given:
1. "quit" exists from screen
2. "readreg regname filename" reads a file into a named "regname" storage location in screen.
3. "paste regname" pastes the contents of the named "regname" storage location into the screen session.

The readreg and paste commands can be used to send the contents into a ZynqShell array when using the ZynqShell loadArray command.

4. "help" displays screen help. 
