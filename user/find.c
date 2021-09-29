#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//
// If path param is a directory, find and print path of the file with given name (fname)
// inside the given directory and it's subdirectories using recursion.
// If given path is not a directory, print out message informing user that given path is not dir
//
void
find(char *path, char *fname)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot fstat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
    case T_DIR:
      if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
	fprintf(2, "find: path too long\n");
	break;
      }
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';

      // for each valid de.name do: if its a file, and it's name equals fname, print it's path.
      // else if its a dir, call find with it's path
      while(read(fd, &de, sizeof(de)) == sizeof(de)){
	if(de.inum != 0 && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0){
	  memmove(p, de.name, DIRSIZ);
	  p[DIRSIZ] = 0;
	  if(stat(buf, &st) < 0){
	    fprintf(2, "find: cannot stat\n");
	    continue;
	  }
	  switch(st.type){
	    case T_DIR:
	      find(buf, fname);
	      break;

	    case T_FILE:
	      if(strcmp(de.name, fname) == 0){
		printf("%s\n", buf);
	      }
	      break;
	  }
	}
      }
    break;

    default:
      fprintf(2, "find: %s is not directory\n", path);
      break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2,"usage: find <dir> <fname>\n");
    exit(1);
  }

  find(argv[1], argv[2]);
  exit(0);
}
