/*******************************************************************************
 *
 * REVOLABS CONFIDENTIAL
 * __________________
 *
 *  [2005] - [2016] Revolabs Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Revolabs Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Revolabs Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Revolabs Incorporated.
 *
 ******************************************************************************/

/**
 * @file AudioFwk.c
 * @brief Audio processing frame work
 * @version $LastChangedRevision:$
 * @author Ryo Tanaka
 */
#define _GNU_SOURCE  
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <jack/jack.h>
#include <sys/types.h>
#include <unistd.h>

#include "RevoRT.h"
#include "FixedHeap.h"
#include "AudioFwk.h"

#if 0
#define AUDIOFWK_MEM_ALLOC(pool, type, count, alignment) MEM_ALLOC(pool, type, count, alignment)
#define AUDIOFWK_MEM_FREE(ptr) 
#else
#define AUDIOFWK_MEM_ALLOC(pool, type, count, alignment) (type*)calloc(count, sizeof(type))
#define AUDIOFWK_MEM_FREE(ptr) free(ptr)
#endif

struct AudioFwkState_ {
  int                       nch_input;
  int                       nch_output;
  int                       frame_size;
  AUDIOFWK_PROCESS_CALLBACK process_callback;
  jack_client_t *           client;
  jack_port_t **            input_ports;
  jack_port_t **            output_ports;
  float **                  input_buffers;
  float **                  output_buffers;

  /* Task handler */
  char             tskName[32];
  MBX_Handle       mbx;
  TSK_Handle       tsk;
  TaskDescriptor_t tskDescriptor;  
};

/* command format of Mailbox */
typedef struct {
  jack_nframes_t nframes;
} cmd_desc_t;

/*----------------------------------
    Task of AudioFwk
 -----------------------------------*/
static void
process_task(void* arg1)
{
  static Environment_t env;
  cmd_desc_t cmd;
  AudioFwkState *st = (AudioFwkState*)arg1;

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(3, &cpu_set);
  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);

  TRACE(LEVEL_INFO, "Task AudioFwk started on CPU %d", sched_getcpu());
  
  InitEnvironment(&env, 128*1024, __func__);

  do {
    MBX_pend(st->mbx, &cmd, SYS_FOREVER);

    // Not needed because not a real time task
    Task_startNewCycle();

    if (st->process_callback) {
      st->process_callback(st->output_buffers, st->input_buffers);
    }

  } while(1);
}


/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

static int
process_callback( jack_nframes_t nframes, void *arg )
{
  int i;
  AudioFwkState* st = (AudioFwkState*)arg;
  cmd_desc_t cmd;

  /* set each buffer pointer */
  for (i=0; i<st->nch_input; i++) {
    st->input_buffers[i] = (float*)(jack_port_get_buffer ( st->input_ports[i], nframes ));
  }
  for (i=0; i<st->nch_output; i++) {
    st->output_buffers[i] = (float*)(jack_port_get_buffer ( st->output_ports[i], nframes ));
  }

  cmd.nframes = nframes;
  MBX_post(st->mbx, &cmd, 0);

  return 0;
}

void audiofwk_close(AudioFwkState* st)
{
  if (st) {
    if (st->client) {
      jack_deactivate(st->client);
      jack_client_close(st->client);
    }
    if (st->input_ports)  AUDIOFWK_MEM_FREE(st->input_ports);
    if (st->output_ports) AUDIOFWK_MEM_FREE(st->output_ports);
    if (st->input_ports)  AUDIOFWK_MEM_FREE(st->input_buffers);
    if (st->output_ports) AUDIOFWK_MEM_FREE(st->output_buffers);
    if (st)               AUDIOFWK_MEM_FREE(st);
  }
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
audiofwk_shutdown ( void *arg )
{
  AudioFwkState* st = (AudioFwkState*)arg;
  audiofwk_close(st);
  exit ( 1 );
}

AudioFwkState* audiofwk_init(const char * name, int nch_input, int nch_output, int frame_size, AUDIOFWK_PROCESS_CALLBACK callback)
{
  int i;
  AudioFwkState *st = NULL;
  const char **ports;
  const char *client_name;
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  /* create instance and store arguments */
  st = AUDIOFWK_MEM_ALLOC(MEM_SDRAM, AudioFwkState, 1, 4);
  st->nch_input        = nch_input;
  st->nch_output       = nch_output;
  st->frame_size       = frame_size;
  st->process_callback = callback;

  /* initialize mailbox */
  st->mbx = MBX_create(sizeof(cmd_desc_t), 1, NULL);
  /* initialize task */
  sprintf(st->tskName, "TaskAudioFwk");
  st->tskDescriptor.h        = &st->tsk;
  st->tskDescriptor.f        = (Fxn)process_task;
  st->tskDescriptor.name     = st->tskName;
  st->tskDescriptor.stack    = 5*1024;
  st->tskDescriptor.priority = 10;
  st->tskDescriptor.arg1     = st;
  Task_init(&st->tskDescriptor, 1);
  
  /* open a client connection to the JACK server */
  st->client = jack_client_open ( name, options, &status, server_name );
  if ( st->client == NULL )    {
    fprintf ( stderr, "jack_client_open() failed, "
	      "status = 0x%2.0x\n", status );
    if ( status & JackServerFailed ) {
      fprintf ( stderr, "Unable to connect to JACK server\n" );
    }
    goto ERR_END;
  }
  if ( status & JackServerStarted ) {
      fprintf ( stderr, "JACK server started\n" );
    }
  if ( status & JackNameNotUnique ) {
    client_name = jack_get_client_name ( st->client );
    fprintf ( stderr, "unique name `%s' assigned\n", client_name );
  }
  

  /* tell the JACK server to call `process()' whenever
     there is work to be done.
  */
  jack_set_process_callback ( st->client, process_callback, st);
  
  /* tell the JACK server to call `audiofwk_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */
  jack_on_shutdown ( st->client, audiofwk_shutdown, st );

  /* create ports and buffers */
  st->input_ports    = AUDIOFWK_MEM_ALLOC(MEM_SDRAM, jack_port_t*, st->nch_input,  4);
  st->output_ports   = AUDIOFWK_MEM_ALLOC(MEM_SDRAM, jack_port_t*, st->nch_output, 4);
  st->input_buffers  = AUDIOFWK_MEM_ALLOC(MEM_SDRAM, float*,       st->nch_input,  4);
  st->output_buffers = AUDIOFWK_MEM_ALLOC(MEM_SDRAM, float*,       st->nch_output, 4);

  /* Name each ports */
  char port_name[20];
  for (i=0; i<st->nch_input; i++) {
    sprintf ( port_name, "input_%d", i + 1 );
    st->input_ports[i] = jack_port_register ( st->client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
    if ( st->input_ports[i] == NULL ) {
      fprintf ( stderr, "no more JACK ports available\n" );
      goto ERR_END;
    }
  }
  for (i=0; i<st->nch_output; i++) {
    sprintf ( port_name, "output_%d", i + 1 );
    st->output_ports[i] = jack_port_register ( st->client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    if ( st->output_ports[i] == NULL ) {
      fprintf ( stderr, "no more JACK ports available\n" );
      goto ERR_END;
    }
  }
  
  
  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */

  if ( jack_activate ( st->client ) ) {
    fprintf ( stderr, "cannot activate client" );
    goto ERR_END;
  }

  /* Connect the ports.  You can't do this before the client is
   * activated, because we can't make connections to clients
   * that aren't running.  Note the confusing (but necessary)
   * orientation of the driver backend ports: playback ports are
   * "input" to the backend, and capture ports are "output" from
   * it.
   */

  ports = jack_get_ports ( st->client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput );
  if ( ports == NULL ) {
    fprintf ( stderr, "no physical capture ports\n" );
    goto ERR_END;
  }

  for (i=0; i<st->nch_input; i++) {
    if ( jack_connect ( st->client, ports[i], jack_port_name ( st->input_ports[i] ) ) ) {
      fprintf ( stderr, "cannot connect input ports\n" );
    }
  }

  free ( ports );

  ports = jack_get_ports ( st->client, NULL, NULL, JackPortIsPhysical|JackPortIsInput );
  if ( ports == NULL ) {
    fprintf ( stderr, "no physical playback ports\n" );
    goto ERR_END;
  }

  for (i=0; i<st->nch_output; i++) {
    if ( jack_connect ( st->client, jack_port_name ( st->output_ports[i] ), ports[i] ) ) {
      fprintf ( stderr, "cannot connect output ports\n" );
    }
  }

  free ( ports );

  return st;
  
 ERR_END:
  if (st) audiofwk_close(st);
  return NULL;
}
