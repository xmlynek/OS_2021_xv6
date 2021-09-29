#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

//
// easier implementation of xargs that reads lines from standard input and executes command for each line
// and the given line will be used as arguments to that command.
//
int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "usage: xargs command [args]\n");
    exit(1);
  }
  char *xargsv[MAXARG];
  int xargsv_count = argc-1;
  char buf[512];
  int buf_index = 0;
  char chr;
  int arg_index = 0;

  // copy args from argv to xargsv without argv[0]
  // these args will be passed to exec
  for(int i = 0; i < argc - 1; i++){
    xargsv[i] = argv[i+1];
  }

  while(read(0, &chr, 1) > 0){
    // if space or new line was read, add pointer on the beggining of the arg to xargsv
    // otherwise keep 'building' the argument
    if(chr == ' ' || chr == '\n'){
      buf[buf_index++] = 0;
      xargsv[xargsv_count++] = &buf[arg_index];
      arg_index = buf_index;
    } else{
      buf[buf_index++] = chr;
      continue;
    }

    // if \n was read, start new process and execute given command with given args(xargsv)
    if(chr == '\n'){
      int pid = fork();
      if(pid == 0){
	// child
	exec(xargsv[0], xargsv);
	// exec should call exit(), if not, error may have occured
	exit(1);
      }	else if(pid > 0){
	// parent
	wait(0);
      } else {
	fprintf(2, "xargs: fork failed\n");
	exit(1);
      }
      // reset xargsv_count back to beginning - 'clear' old arguments(xargsv)
      xargsv_count = argc - 1;
    }
  }
  exit(0);
}
