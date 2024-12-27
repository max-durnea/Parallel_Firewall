// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	ring->data = (char *)malloc(sizeof(char) * cap);
	if (!ring->data)
		return -1;

	ring->len = 0;
	ring->cap = cap;
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->stop_flag = 0;

	pthread_mutex_init(&ring->mutex, NULL);
	pthread_cond_init(&ring->write_cond, NULL);
	pthread_cond_init(&ring->read_cond, NULL);

	return 0;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(&ring->mutex);

	while (ring->len == ring->cap)
		pthread_cond_wait(&ring->write_cond, &ring->mutex);

	size_t space_left = ring->cap - ring->write_pos;

	if (size <= space_left)
		memcpy(ring->data + ring->write_pos, data, size);
	else
		memcpy(ring->data, (char *)data, size);
	ring->write_pos = (ring->write_pos + size) % ring->cap;
	ring->len += size;

	pthread_cond_signal(&ring->read_cond);
	pthread_mutex_unlock(&ring->mutex);

	return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	size_t total_read = 0;

	while (size > 0) {
		pthread_mutex_lock(&ring->mutex);

		while (ring->len == 0 && !ring->stop_flag)
			pthread_cond_wait(&ring->read_cond, &ring->mutex);

		if (ring->len == 0 && ring->stop_flag) {
			pthread_mutex_unlock(&ring->mutex);
			return total_read;
		}
		memcpy((char *)data + total_read, ring->data + ring->read_pos, size);

		ring->read_pos = (ring->read_pos + size) % ring->cap;
		ring->len -= size;

		total_read += size;
		size -= size;

		pthread_mutex_unlock(&ring->mutex);
		pthread_cond_signal(&ring->write_cond);
	}
	return total_read;
}


void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	free(ring->data);

	pthread_mutex_destroy(&ring->mutex);
	pthread_cond_destroy(&ring->read_cond);
	pthread_cond_destroy(&ring->write_cond);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	pthread_mutex_lock(&ring->mutex);
	ring->stop_flag = 1;

	pthread_cond_broadcast(&ring->read_cond);
	pthread_mutex_unlock(&ring->mutex);
}
