#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//
// Argument(left_fd) represents file descriptor of pipe for reading from previous(parent's) process.
// The number(prime) that was read from pipe using pipe's read file descriptor, is declared as prime number.
// Actuall process is reading numbers(num) from pipe until the pipe is not empty or closed.
// If num is divisible by prime, then do nothing bcs its not a prime number.
// Otherwise check if there is already created pipe to the right side(to new process with the next prime).
// If not, create it, then write num to the right side. If it is, just write num to the right side.
//
void
primes(int left_fd)
{
  int right[2];
  int right_exists  = 0;
  int prime;
  int num;

  if(read(left_fd, &prime, sizeof(prime)) <= 0){
   fprintf(2,"primes: read EOF or error\n");
   return;
  }
  printf("prime %d\n", prime);

  while(read(left_fd, &num, sizeof(num)) > 0){
    if(num % prime != 0){
      if(!right_exists){
	if(pipe(right) == -1){
	  fprintf(2,"primes: error creating pipe\n");
	  exit(1);
	}
	right_exists = 1;

	int pid = fork();
	if(pid == 0){
	  // close unused file descriptors in new process
	  close(left_fd);
	  close(right[1]);
	  primes(right[0]);
	  return;
	} else if(pid < 0){
	  fprintf(2,"primes: can not fork\n");
	  exit(1);
	}
      }
      write(right[1], &num, sizeof(num));
    }
  }
  close(right[1]);
  close(right[0]);
  close(left_fd);
}

int
main()
{
  int right[2];

  if(pipe(right) == -1){
    fprintf(2, "error creating pipe\n");
    exit(1);
  }
  for(int i = 2; i <= 35; i++){
    write(right[1], &i, sizeof(i));
  }
  close(right[1]);
  primes(right[0]);

  wait(0);
  exit(0);
}
