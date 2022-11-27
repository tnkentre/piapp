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
 * @file MidiFwk.c
 * @brief Midi processing frame work
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
#include <jack/midiport.h>

#include "RevoRT.h"
#include "FixedHeap.h"
#include "MidiFwk.h"

#if 0
#define MIDIFWK_MEM_ALLOC(pool, type, count, alignment) MEM_ALLOC(pool, type, count, alignment)
#define MIDIFWK_MEM_FREE(ptr) 
#else
#define MIDIFWK_MEM_ALLOC(pool, type, count, alignment) (type*)calloc(count, sizeof(type))
#define MIDIFWK_MEM_FREE(ptr) free(ptr)
#endif

#define MBX_LENGTH 1024

struct MidiFwkState_ {
  MIDIFWK_PROCESS_CALLBACK  process_callback;
  jack_client_t *           client;
  jack_port_t *             input_port;
  jack_port_t *             output_port;
  uint64_t                  monotonic_cnt;

  /* Task handler */
  char             tskName[32];
  MBX_Handle       mbx;
  TSK_Handle       tsk;
  TaskDescriptor_t tskDescriptor;  
};

/* command format of Mailbox */
typedef struct {
  uint64_t monotonic_cnt;
  uint32_t time;
  uint8_t  type;
  uint8_t  channel;
  uint8_t  param;
  uint8_t  value;
} cmd_desc_t;

/*----------------------------------
    Task of MidiFwk
 -----------------------------------*/
static void
process_task(void* arg1)
{
  static Environment_t env;
  cmd_desc_t cmd;
  MidiFwkState *st = (MidiFwkState*)arg1;

  TRACE(LEVEL_INFO, "Task MidiFwk started on CPU %d", sched_getcpu());
  
  InitEnvironment(&env, 128*1024, __func__);

  do {
    MBX_pend(st->mbx, &cmd, SYS_FOREVER);

    // Not needed because not a real time task
    Task_startNewCycle();

    if (st->process_callback) {
      st->process_callback(cmd.monotonic_cnt,
			   cmd.time,
			   cmd.type,
			   cmd.channel,
			   cmd.param,
			   cmd.value);
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
  MidiFwkState* st = (MidiFwkState*)arg;
  void* buffer;
  jack_nframes_t N;
  jack_nframes_t i;

  /* get receive buffer */
  buffer = jack_port_get_buffer (st->input_port, nframes);
  assert (buffer);

  N = jack_midi_get_event_count (buffer);
  for (i = 0; i < N; ++i) {
    jack_midi_event_t event;
    cmd_desc_t cmd;
    if ( !jack_midi_event_get (&event, buffer, i) ) {
      if (event.size >= 3) {
	cmd.monotonic_cnt = st->monotonic_cnt;
	cmd.type          = event.buffer[0] & 0xf0;
	cmd.channel       = event.buffer[0] & 0x0f;
	cmd.param         = event.buffer[1];
	cmd.value         = event.buffer[2];
	MBX_post(st->mbx, &cmd, 0);
      }
    }
  }
  st->monotonic_cnt += nframes;
  return 0;
}

void midifwk_close(MidiFwkState* st)
{
  if (st) {
    if (st->client)       jack_client_close(st->client);
    MIDIFWK_MEM_FREE(st);
  }
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
midifwk_shutdown ( void *arg )
{
  MidiFwkState* st = (MidiFwkState*)arg;
  midifwk_close(st);
  exit ( 1 );
}

MidiFwkState* midifwk_init(const char * name, MIDIFWK_PROCESS_CALLBACK callback)
{
  MidiFwkState *st = NULL;
  const char **ports;
  const char *client_name;
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  /* create instance and store arguments */
  st = MIDIFWK_MEM_ALLOC(MEM_SDRAM, MidiFwkState, 1, 4);
  st->process_callback = callback;

  /* initialize mailbox */
  st->mbx = MBX_create(sizeof(cmd_desc_t), MBX_LENGTH, NULL);
  /* initialize task */
  sprintf(st->tskName, "TaskMidiFwk");
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
  
  /* tell the JACK server to call `midifwk_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */
  jack_on_shutdown ( st->client, midifwk_shutdown, st );

  /* Name each ports */
  char port_name[20];
  sprintf ( port_name, "%s_input", name );
  st->input_port = jack_port_register ( st->client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );
  if ( st->input_port == NULL ) {
    fprintf ( stderr, "no more JACK ports available\n" );
    goto ERR_END;
  }
  sprintf ( port_name, "%s_output", name );
  st->output_port = jack_port_register ( st->client, port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0 );
  if ( st->output_port == NULL ) {
    fprintf ( stderr, "no more JACK ports available\n" );
    goto ERR_END;
  }
  
  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */

  if ( jack_activate ( st->client ) ) {
    fprintf ( stderr, "cannot activate client" );
    goto ERR_END;
  }

  /* !!!Need to be modified below!!! */
  

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

  if ( jack_connect ( st->client, ports[0], jack_port_name ( st->input_port ) ) ) {
    fprintf ( stderr, "cannot connect input ports\n" );
  }

  free ( ports );

  ports = jack_get_ports ( st->client, NULL, NULL, JackPortIsPhysical|JackPortIsInput );
  if ( ports == NULL ) {
    fprintf ( stderr, "no physical playback ports\n" );
    goto ERR_END;
  }

  if ( jack_connect ( st->client, jack_port_name ( st->output_port ), ports[0] ) ) {
    fprintf ( stderr, "cannot connect output ports\n" );
  }

  free ( ports );

  return st;
  
 ERR_END:
  if (st) midifwk_close(st);
  return NULL;
}
