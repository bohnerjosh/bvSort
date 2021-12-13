#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define ARRSIZE 1024*1024*256/4

const int SZ = ARRSIZE;
int buf[ARRSIZE];

int main() {

  int fd = open("rand.data", O_WRONLY | O_CREAT, S_IRWU);

  printf("fd=%d\n", fd);
  printf("errno: %s\n", strerror(errno));

  for(int q=0; q < 16; q++) {
    for(int i=0; i < SZ; i++) {
      buf[i] = rand();
    }


    int numBytes = write(fd, buf, sizeof(buf));
    printf("Wrote %d bytes\n", numBytes);
    printf("errno str: %s\n", strerror(errno));
    printf("errno val: %d\n", errno);
  }

  close(fd);

  return 0;
}

