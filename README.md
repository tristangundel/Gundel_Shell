# Gundel_Shell
A UNIX shell with foreground and background command running capabilities, written in C

## Compilation
A make file is included in the repository, to compile all necessary object files and the final executable run the following command:

```
make
```

## Usage
To begin execution within the gundelsh interface, simply run the executable created by the makefile using the following command from within the project directory:

```
$ ./gundelsh
```

Successful entrance into the gundelsh interface is indicated by a result of a leading ':' in the prompt of the command line:

```
$ ./gundelsh
:
```

### Exiting the Shell
To exit the interface, type '**exit**' into the command-line prompt and hit ENTER.

### Running Foreground Commands
To execute commands within the foreground (where control will not return to the user until the program exits) type the name of the executable you'd like to run, followed by any arguments you would like to provide, and then press ENTER. Local executables as well as any executables within your environment's PATH variable can be run from within the shell. Example provided below:

```
: ls ./testDir
testFileOne.txt    testFileTwo.txt    testFileThree.txt
: 
```
Any command entered into the prompt can reference the process id of the shell itself by including the character sequence '**$$**'. Example provided below:
```
: echo This is the shell's process id: $$
This is the shell's process id: 55234
:
```
Foreground commands can be terminated at any time by pressing CTRL-C.

### Redirecting stdin and stdout
To redirect stdin for a program to be executed, include a **<** character and the **filename** to replace stdin (separated by a space) at the very end of the command. Example shown below:

```
: echo < testFile.txt
This is the content of testFile.txt
:
```
To redirect stdout for a program to be executed, include a **>** character and the **filename** to replace stdin (separated by a space) at the very end of the command. Example shown below:

```
: ls ./testDir > testFile.txt
:
```

### Running Background Commands
To execute commands within the background (where control will return to the user immediately) type the name of the executable you'd like to run, followed by any arguments you would like to provide and a trailing '**&**' character at the very end as the last argument, and then press ENTER. The process id of the program executing will then be printed, in case it must be accessed at a later time by the user. By default, stdout is redirected to '/dev/null' for all background processes unless a different redirection location is specified. Local executables, as well as any executables within your environment's PATH variable can be run from within the shell. Example provided below:

```
: ls ./testDir &
background pid is 54770
:
```
Upon completion of the background process, a message will be displayed to notify the user of the program's exit status

### Foreground-only Mode
A user input of CTRL-Z will place the shell in foreground only mode. In this mode, background commands will not be permitted. Example provided below:
```
: ^Z
Entering foreground-only mode (& is now ignored)
: echo This is a test &
This is a test
:
```
To exit foreground-only mode, press CTRL-Z.

### Checking Status
A user input of '**status**' will print out the exit value of the most recently completed program executed from within the shell. Example provided below:
```
: status
exit value: 0
:
```
