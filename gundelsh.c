/* Author: Tristan Gundel
 * Date: 11/21/2019
 * Description: This program will execut a simple shell for execution in
 *              a UNIX system
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

//  defining variables to hold maximum values for characters, arguments,
//  and concurrently running background processes
#define MAXARGS 512
#define MAXCHARS 2049    //  I included an extra space for the newline character
                         //  added by getline. If that was meant to be included
                         //  in the 2048 max, this would be changed to 2048

#define MAXPIDS 100

//global variable to be used in SIGTSTP signal handler
int fg;

//signal handler function for SIGTSTP
void Catch_SIGTSTP(int sigNumber){
  //if not in foreground-only mode, enter foreground-only mode
  if (!fg){
    write(1, "\nEntering foreground-only mode (& is now ignored)\n: ", 52);
    fg = 1;
  } else {    // else exit foreground-only mode
    write(1, "\nExiting foreground-only mode\n: ", 32);
    fg = 0;
  }
  return;
}

//  function to exit program when called
void Exit_Function(pid_t* myChildren, int numOfChildren){

  int i=0;
  for (i=0; i<numOfChildren; i++){
    kill(myChildren[i], SIGTERM);      // kill all currently running processes
  }
  //exit shell
  exit(0);
}

//  function to return the status variable
void Status_Function(int* status){

  printf("exit value %d\n", *status);
  fflush(stdout);    //flushstdout after printing status update
  return;

}

//  function to change directory upon user's request
void CD_Function(char** arguments){

  //  declaring variable to hold HOME environment variable
  char* userHOME = getenv("HOME");

  //  store result of directory change in variable to check for error
  int chdirResult;
  if (arguments[1] == NULL){
    chdir(userHOME);        //  with no extra argument, change to home directory
  } else {
    //  otherwise, change to directory specified
    chdirResult = chdir(arguments[1]);

    //  if path is invalid, print error message
    if (chdirResult == -1){
      printf("that is not a valid path\n");
      fflush(stdout);
    }
  }
}

//  function to execute a foreground command in the shell
void Execute_Command(char** arguments, int numOfArgs, int* status){

  //  declare variable to be used within the switch/case statements
  pid_t pid;     //  will hold new process' pid
  int i, j;     //  looping variables
  int childExitMethod;  //  stores exit status from waitpid
  int inputFile = 0;    //  future input file descriptor
  int outputFile = 0;   //  fiture output file descriptor
  int result;      // error checking variable for exec commands

  // fork to a new process
  pid = fork();

  //  signal handling structs to default, and ignore
  struct sigaction defaultInterrupt = {{0}};
  struct sigaction ignoreAction = {{0}};


  switch(pid){
    //  if error occurs with fork, exit process
    case -1:
      exit(1);
      break;
    //  child process instructions
    case 0:
      //  loop to search for < and > characters for redirection
      for(i=0; i<numOfArgs; i++){
        //  conditional for redirection of input
        if(arguments[i] != NULL && strcmp(arguments[i], "<") == 0){
          //  open file to the right of redirection operator
          inputFile = open(arguments[i+1], O_RDONLY, 0666);
          //  check to ensure that file opened
          if (inputFile < 0) {
            fprintf(stderr, "cannot open %s for input\n", arguments[i+1]);
            fflush(stderr);
            //set status variable
            *status = 1;
            exit(1);
          } else {
            //  remove redirection arguments, and later arguments backwards two
            for (j=i; j<numOfArgs; j++){
              if(arguments[j+2] == NULL){
                arguments[j] = NULL;
              } else {
                strcpy(arguments[j], arguments[j+2]);
              }
            }
            //  reduce looping variable once, and lower number of arguments
            i--;
            numOfArgs--;
            numOfArgs--;
          }

          //  conditional for redirection of output
        } else if (arguments[i] != NULL && strcmp(arguments[i], ">") == 0){
          //  open file to the right of redirection operator
          outputFile = open(arguments[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
          // check to ensure file opened correctly
          if (outputFile < 0){
            fprintf(stderr, "cannot open %s for output\n", arguments[i+1]);
            fflush(stderr);
            //  set status variable
            *status = 1;
            exit(1);
          } else {
            //  remove redirection arguments, and later arguments backwards two
            for (j=i; j<numOfArgs; j++){
              if(arguments[j+2] == NULL){
                arguments[j] = NULL;
              } else {
                strcpy(arguments[j], arguments[j+2]);
              }
            }
            //  reduce looping variable once, and lower number of arguments
            i--;
            numOfArgs--;
            numOfArgs--;
          }
        }
      }
      //redirect stdout if redirection was called for
      if (outputFile) {
        dup2(outputFile, 1);
      }
      //redirect stdin if redirection was called for
      if (inputFile) {
        dup2(inputFile, 0);
      }

      // set signal handling options for child process
      defaultInterrupt.sa_handler = SIG_DFL;
      ignoreAction.sa_handler = SIG_IGN;
      sigaction(SIGINT, &defaultInterrupt, NULL);
      sigaction(SIGTSTP, &ignoreAction, NULL);

      // call new command for child process, and check for error
      result = execvp(arguments[0], arguments);
      if (result == -1){
        *status = 1;
        fprintf(stderr, "No such command found\n");
        fflush(stderr);
      }
      exit(1);
      break;
    //  parent process instructions
    default:
      //  wait for foreground process to complete
      waitpid(pid, &childExitMethod, 0);

      //  check for child process exit method/status
      if (WIFEXITED(childExitMethod) != 0){
        *status = WEXITSTATUS(childExitMethod);
      } else if (WIFSIGNALED(childExitMethod) != 0){
        int termSignal = WTERMSIG(childExitMethod);
        printf("terminated by signal %d\n", termSignal);
        fflush(stdout);
      }
  }
  return;
}

//  function to execute a background command in the shell
int Execute_Background_Command(char** arguments, int numOfArgs, int* status){

  //  declare variable to be used within the switch/case statements
  pid_t pid;     //  will hold new process' pid
  int i, j;     //  looping variables
  int devNull = 0;      //  file descriptor for redirection to /dev/null
  int inputFile = 0;    //  future input file descriptor
  int outputFile = 0;   //  fiture output file descriptor
  int result;      // error checking variable for exec commands

  //  fork to a new process
  pid = fork();

  //  signal handling struct for ignoring signal
  struct sigaction ignoreAction = {{0}};

  switch(pid){
    //  if error occurs with fork, exit the process
    case -1:
      exit(1);
      break;
    //  instructions for the child process
    case 0:
      //  loop to search for < and > characters for redirection
      for(i=0; i<numOfArgs; i++){
        //  conditional for redirection of input
        if(arguments[i] != NULL && strcmp(arguments[i], "<") == 0){
          //  open file to the right of redirection operator
          inputFile = open(arguments[i+1], O_RDONLY, 0666);
          //  check to ensure that file opened
          if (inputFile < 0) {
            fprintf(stderr, "cannot open %s for input\n", arguments[i+1]);
            fflush(stderr);
            //set status variable
            *status = 1;
            exit(1);
          } else {
            //  remove redirection arguments, and later arguments backwards two
            for (j=i; j<numOfArgs; j++){
              if(arguments[j+2] == NULL){
                arguments[j] = NULL;
              } else {
                strcpy(arguments[j], arguments[j+2]);
              }
            }
            //  reduce looping variable once, and lower number of arguments
            i--;
            numOfArgs--;
            numOfArgs--;
          }

          //  conditional for redirection of output
        } else if (arguments[i] != NULL && strcmp(arguments[i], ">") == 0){
          //  open file to the right of redirection operator
          outputFile = open(arguments[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
          // check to ensure file opened correctly
          if (outputFile < 0){
            fprintf(stderr, "cannot open %s for output\n", arguments[i+1]);
            fflush(stderr);
            //  set status variable
            *status = 1;
            exit(1);
          } else {
            //  remove redirection arguments, and later arguments backwards two
            for (j=i; j<numOfArgs; j++){
              if(arguments[j+2] == NULL){
                arguments[j] = NULL;
              } else {
                strcpy(arguments[j], arguments[j+2]);
              }
            }
            //  reduce looping variable once, and lower number of arguments
            i--;
            numOfArgs--;
            numOfArgs--;
          }
        }
      }
      //  signal handler assignments
      ignoreAction.sa_handler = SIG_IGN;
      sigaction(SIGINT, &ignoreAction, NULL);
      sigaction(SIGTSTP, &ignoreAction, NULL);

      // conditional statements for redirecting input/output
      if (!outputFile && !inputFile){
        devNull = open("/dev/null", O_RDWR);
        dup2(devNull, 1);    //  redirect input/output to /dev/null by default
        dup2(devNull, 0);
      } else if (outputFile && !inputFile) {
        devNull = open("/dev/null", O_RDONLY);
        dup2(devNull, 0);      //  redirect output only
        dup2(outputFile, 1);

      } else if (inputFile && !outputFile) {
        devNull = open("/dev/null", O_WRONLY);
        dup2(devNull, 1);      //  redirect input only
        dup2(inputFile, 0);
      } else {
        dup2(inputFile, 0);     //  redirect both input/output if necessary
        dup2(outputFile, 1);
      }

      //  execute command and check for error result
      result = execvp(arguments[0], arguments);
      if (result == -1){
        *status = 1;
        fprintf(stderr, "No such command found\n");
        fflush(stderr);
      }
      exit(1);
      break;
      //  instructions for the parent process
    default:
      //  print pid of background process that started
      printf("background pid is %ld\n", (long)pid);
      fflush(stdout);
      return pid;
  }

}

//  function to parse input into arguments
//  and check the user input for built-in commands, or comments
void Check_Input(char* input, int* status, pid_t* myChildren, int* numOfChildren, int fgOnly){

  //  allocate memore for the arguments in command and set them all to NULL
  char* arguments[MAXARGS];
  int i;
  for (i=0; i<MAXARGS; i++){
    arguments[i] = (char*)malloc(100);
    arguments[i] = NULL;
  }
  //  declare variable to hold new background PIDs
  int newBackground;

  //  loop to get through all of user input and put arguments into array
  int count = 0;
  arguments[count] = strtok(input, " ");
  while (arguments[count] != NULL){
    char *pidLocation = NULL;
    int replacedPID = 0;
    char *nextArg = NULL;
    //  check for $$ variable
    pidLocation = strstr(arguments[count], "$$");
    //  if found, change "$$" into pid within argument
    if (pidLocation != NULL){
     		
          char newString[MAXCHARS];
          memset(newString, '\0', MAXCHARS);
          int stringCount = 0;
          if (pidLocation != arguments[count]){
            for(i=0; i<(pidLocation - arguments[count]); i++){
              newString[i] = arguments[count][i];
              stringCount++;
            }
          }
          stringCount = strlen(newString);
          char pidString[10];
          sprintf(pidString, "%d", getpid());
          strcat(newString, pidString);
          if (stringCount + 2 < strlen(arguments[count])){
            strcat(newString, &arguments[count][stringCount+2]);
          }
          nextArg = strtok(NULL, " ");
          replacedPID = 1;
          strcpy(arguments[count], newString);
    }
    
    count++;
    if (replacedPID){
      if (nextArg == NULL){
        arguments[count] = NULL;
      } else { 
        strcpy(arguments[count], nextArg);
      }
      replacedPID = 0;
    } else {
      arguments[count] = strtok(NULL, " ");
    }
  }
  //  check amount of arguments entered and return error message if exceeds max
  if (count == MAXARGS && strtok(NULL, " ") != NULL){
    printf("only %d arguments allowed, try again", MAXARGS);
    fflush(stdout);
    return;
  }

  //  conditional statements to handle command given
  if (strcmp(arguments[0], "exit") == 0){
    Exit_Function(myChildren, *numOfChildren);    // execute exit command
    return;
  } else if (strcmp(arguments[0], "status") == 0){
    Status_Function(status);                   //  execute status command
    return;
  } else if (strcmp(arguments[0], "cd") == 0){
    CD_Function(arguments);               //  execute directory change command
    return;
  } else if (arguments[0][0] != '#'){  //  if command was not a comment
    //  conditional for background commands
    if (strcmp(arguments[count - 1], "&") == 0){
      //  discard last argument (&)
      count--;
      arguments[count] = NULL;
      //  check for foreground-only mode
      if (!fgOnly){
        newBackground = Execute_Background_Command(arguments, count, status);
        if (newBackground != -1){
          //  add new pid to children array
          myChildren[*numOfChildren] = newBackground;
          *numOfChildren += 1;
        }
      //  in foreground-only, don't do background command funtion
      } else {
        Execute_Command(arguments, count, status);
      }
      // for foregroun command, execute foreground function
    } else {
      Execute_Command(arguments, count, status);
    }
  }
  // free up argument memory
  for (i=0; i<count; i++){
    free(arguments[count]);
  }

  return;
}

int main() {

  //  declare array to hole current background process PIDs
  pid_t myChildren[MAXPIDS];
  int numOfChildren = 0;

  //  initially set foreground-only mode to "off"
  fg = 0;

  //  signal struct handler for TSTP
  struct sigaction SIGTSTP_action = {{0}};
  SIGTSTP_action.sa_handler = Catch_SIGTSTP;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = SA_RESTART;

  //  signal handler struct to ingnore signals
  struct sigaction ignore_action = {{0}};
  ignore_action.sa_handler = SIG_IGN;

  //  set signal handler functions to trigger on signal reception
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);
  sigaction(SIGINT, &ignore_action, NULL);

  //  declare variables to be used for user input in main shell loop
  char* userInput = NULL;
  size_t bufferSize = 0;
  int inputSize;
  int runLoop = 1;   //  set to 'true' to control endless loop
  int status = 0;    //  set intial status to 0

  while(runLoop){

    //  declare variables used to report and terminate
    //  finished background processes
    int backgroundPID;
    int i;
    int moveBack = 0;
    int childExitStatus;
    //  loop over children array of pids and check for status
    for (i=0; i<numOfChildren; i++){
      backgroundPID = -1;  //  set to negative before calling waitpid
      backgroundPID = waitpid(myChildren[i], &childExitStatus, WNOHANG);
      myChildren[i-moveBack] = myChildren[i];
      //  conditional for if background process has exited
      if (backgroundPID == myChildren[i]){
        moveBack++;  //  increase number of spaces to move pids back in array

        //  check exit status for background process that exited
        if (WIFEXITED(childExitStatus) != 0){
          int exitStatus = WEXITSTATUS(childExitStatus);
          printf("\nbackground pid %d is done: exit value %d\n", backgroundPID, exitStatus);
          fflush(stdout);
        } else if (WIFSIGNALED(childExitStatus) != 0){
          int termSignal = WTERMSIG(childExitStatus);
          printf("\nbackground pid %d is done: exit value %d\n", backgroundPID, termSignal);
          fflush(stdout);
        }
        //  ensure that the process was killed
        kill(backgroundPID, SIGTERM);
      }
    }

    //flush output buffers
    fflush(stdout);
    fflush(stderr);
    fflush(stdin);

    bufferSize = 0;   //  set buffer size to zer before getline
    printf(": ");     //  print colon for prompt
    fflush(stdout);

    // retrieve user input with getline
    inputSize = getline(&userInput, &bufferSize, stdin);

    //  if getline returned an error, try again
    if (inputSize == -1){
      clearerr(stdin);
      continue;

      //  check to ensure that input was not over max characters
    } else if (inputSize > MAXCHARS) {
      printf("only %d characters allowed for this shell, try again", MAXCHARS);
      fflush(stdout);

      // check for blank entry by user
    } else if (strcmp(userInput,"\n") == 0){
      continue;

      //  otherwise, check input for which command to run
    } else {
      userInput[inputSize -1] = '\0';
      Check_Input(userInput, &status, myChildren, &numOfChildren, fg);
    }
  }

  // free allocated memory for user input
  free(userInput);
  return 0;
}
