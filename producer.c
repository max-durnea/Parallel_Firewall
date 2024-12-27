// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"
#include "producer.h"

void publish_data(so_ring_buffer_t *ring, const char *input_filename)
{
	int in_fd = open(input_filename, O_RDONLY);

	DIE(in_fd < 0, "open input file");

	char buffer[PKT_SZ];
	ssize_t sz;

	while ((sz = read(in_fd, buffer, PKT_SZ)) != 0) {
		DIE(sz != PKT_SZ, "packet truncated");

		// Enqueue the packet into the ring buffer
		ring_buffer_enqueue(ring, buffer, PKT_SZ);
	}

	// Set the done flag and signal consumers
	ring_buffer_stop(ring);

	close(in_fd);
}
