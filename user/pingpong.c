#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//
// communication between parent and child processes using pipes
//
int
main()
{
  int to_child[2];
  int to_parent[2];

  if(pipe(to_child) == -1 || pipe(to_parent) == -1){
    fprintf(2,"Error creating pipe\n");
    exit(1);
  }

  int pid = fork();

  if(pid == 0){
    // child's process
    char received;
    if(read(to_child[0], &received, 1) == 0){
      fprintf(2,"child: read reached EOF!\n");
      exit(1);
    }
    printf("%d: received ping\n", getpid());
    write(to_parent[1], "x", 1);

  } else if (pid > 0){
    // parent's process
    write(to_child[1], "x", 1);
    char received;

    if(read(to_parent[0], &received, 1) == 0){
      fprintf(2, "parent: read reached EOF!\n");
      exit(1);
    }
    printf("%d: received pong\n", getpid());
  } else {
    fprintf(2, "fork error\n");
  }
  exit(0);
}
