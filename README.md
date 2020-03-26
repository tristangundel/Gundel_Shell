# Gundel_Shell
A UNIX shell with foreground and background command running capabilities, written in C

## Compilation
A make file is included in the repository, to compile all necessary object files and the final executable run the following command:

```bash
make
```

## Usage
To begin execution within the gundelsh interface, simply run the executable created by the makefile using the following command from within the project directory:

```bash
./gundelsh
```

Successful entrance into the gundelsh interface is indicated by a result of a leading ':' in the prompt of the command line:

```bash
./gundelsh
:
```

### Exiting the Shell
To exit the interface, type 'exit' into the command-line prompt and hit ENTER.

### Running Foreground Commands
To execute commands within the foreground (where control will not return to the user until the program exits) type the name of the executable you'd like to run, followed by any arguments you would like to provide, and then press ENTER. Example provided below:

```bash
: ls ./testDir

testFileOne.txt    testFileTwo.txt    testFileThree.txt

: 
```
Foreground commands can be terminated at any time by pressing CTRL-C.

### Running Background Commands
To execute commands within the background (where control will return to the user immediately) type the name of the executable you'd like to run, followed by any arguments you would like to provide and a trailing '&' character at the end, and then press ENTER. The process id of the program executing will then be printed, in case it must be accessed at a later time by the user. By default, stdout is redirected to '/dev/null' for all background processes unless a different redirection location is specified. Example provided below:

```bash
: ls ./testDir &
background pid is 54770
:
```
Upon completion of the background process, a message will be displayed to notify the user of the program's exit status

