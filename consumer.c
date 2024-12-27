// SPDX-License-Identifier: BSD-3-Clause
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

//primitive de sincronizare pentru sincronizarea fisierului
struct so_packet_t *shared_buffer;
int buffer_index;
pthread_mutex_t buffer_mutex;
pthread_cond_t buffer_cond;
int num_threads;

void *consumer_thread(void *arg)
{
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)arg;
	so_ring_buffer_t *rb = ctx->producer_rb;


	FILE *outfile = ctx->fp;

	struct so_packet_t *pkt = malloc(sizeof(struct so_packet_t));
	ssize_t bytes_read;

	while (1) {
		bytes_read = ring_buffer_dequeue(rb, pkt, PKT_SZ);
		if (bytes_read == 0)
			break;

		int action = process_packet(pkt);
		unsigned long hash = packet_hash(pkt);
		unsigned long timestamp = pkt->hdr.timestamp;

		fprintf(outfile, "%s %016lx %lu\n", RES_TO_STR(action), hash, timestamp);
		fflush(outfile);
	}

	free(pkt);
	fclose(outfile);
	pthread_exit(NULL);
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	shared_buffer = malloc(num_consumers * sizeof(struct so_packet_t));
	buffer_index = 0;
	pthread_mutex_init(&buffer_mutex, NULL);
	pthread_cond_init(&buffer_cond, NULL);
	num_threads = num_consumers;
	FILE *outfile = fopen(out_filename, "a");

	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

		ctx->producer_rb = rb;
		ctx->out_filename = out_filename;
		ctx->fp = fopen(ctx->out_filename, "a");
		pthread_create(&tids[i], NULL, consumer_thread, ctx);
	}

	return num_consumers;
}
