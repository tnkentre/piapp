#define _GNU_SOURCE  
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <signal.h>

#include <wiringPi.h>

#include "RevoRT.h"
#include "global.h"
#include "FixedHeap.h"
#include "cli.h"

#include "AudioServer.h"
#include "target_cli.h"

#define USE_AUDIOPROC 1

#if USE_AUDIOPROC
#include "AudioProc.h"
#endif

//----------------------------------------------------------
// Memory size definition
//----------------------------------------------------------
int IRAM_MAX_SIZE = 0;
int SHARED_MAX_SIZE = 0;


/** Stubs for local board specific functions */
static void handler(int sig, siginfo_t *si, void *unused)
{
  printf("SIGSEGV at %p\n", si->si_addr);
}

#if USE_AUDIOPROC == 0
pthread_mutex_t syncAudio;

static void pinCallback()
{
  pthread_mutex_unlock(&syncAudio);  
}

void TaskAudio(void)
{
  static Environment_t env;
  
  InitEnvironment(&env, 40*1024, __func__);
  
  do {
    pthread_mutex_lock(&syncAudio);
    
    Task_startNewCycle();
    
    digitalWrite(1, 1);
    delay(1);
    digitalWrite(1, 0);    
  } while(1);
}

static TSK_Handle TskAudio;

#define F(t) (Fxn)t, #t

static TaskDescriptor_t TaskDescriptor[] = {
  { &TskAudio, F(TaskAudio), 80*1024, 70 },
};
#endif /* USE_AUDIOPROC == 0 */

int main()
{
  struct sigaction sa;
  
  FILE *client_to_server;
  char buf[BUFSIZ];

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    printf("Can't install SIGINT handler.\n");
    return -1;
  }

  if (sigaction(SIGHUP, &sa, NULL) == -1) {
    printf("Can't install SIGHUP handler.\n");
    return -1;
  }
  
  printf("Audio Server ON on CPU %d.\n", sched_getcpu());

#if USE_AUDIOPROC == 0
  wiringPiSetup();
  pinMode(0, INPUT);
  pinMode(1, OUTPUT);
  wiringPiISR(0, INT_EDGE_RISING, pinCallback);
#endif
  
  initRevoRT();
  FixedHeap_init();
  cli_init();
  cli_startSerializer();
  cli_target_init();

  global = global_init("gbl");
  
  /* create the FIFO (named pipe) */
  umask(0);
  mknod(FIFO_TO_SERVER, S_IFIFO | 0666, 0);

#if USE_AUDIOPROC
  /* start audio processing with JACK */
  AudioProc_init();
#else
  Task_init(TaskDescriptor, NBELEM(TaskDescriptor));
#endif

  while (1) {
    /* open, read, and display the message from the FIFO */
    client_to_server = fopen(FIFO_TO_SERVER, "r");
    
    fgets(buf, BUFSIZ, client_to_server);
    
    if (strcmp("exit",buf)==0) {
      printf("Server OFF.\n");
      break;
    }
    
    else if (strcmp("",buf) !=0 ) {
      CLI_SERIALIZE(buf);
    }
    
    /* clean buf from any data */
    memset(buf, 0, sizeof(buf));
    
    fclose(client_to_server);
  }

#if USE_AUDIOPROC
  AudioProc_close();
#endif
  
  //  Task_wait();
  
  return 0;
}
