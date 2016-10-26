#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "AudioServer.h"

int main(int argc, char *argv[])
{
  FILE *client_to_server;
  char *cmd;
  
  if (argc != 2) {
    printf("USAGE: CliClient [string]\n");
    exit(1);
  }
  
  /* write str to the FIFO */
  if ((client_to_server = fopen(FIFO_TO_SERVER, "w")) == NULL) {
    perror("fopen");
    exit(1);
  }

  cmd = argv[1];
  
  if (!strncmp(cmd, "dsp ", 4)) {
    fputs(&cmd[4], client_to_server);
  }
  else {
    printf("Cli client: Unimplemented command: %s\n", cmd);
  }
  
  fclose(client_to_server);
  
  /* remove the FIFO */
  
  return 0;
}
