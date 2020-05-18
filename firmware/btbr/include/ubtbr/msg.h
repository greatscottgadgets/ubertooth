#ifndef __DEF_MSG_H
#define __DEF_MSG_H
#include <stdint.h>

typedef struct msg_s {
	uint16_t len;
	uint16_t data_len;
	uint8_t *read;
	uint8_t *write;
	/* never move head */
	uint8_t data[0];
} msg_t;

/* Allocate a msg_t wigh given room*/
static inline msg_t *msg_alloc(unsigned size)
{
	msg_t *msg = (msg_t*)btctl_mem_alloc(sizeof(msg_t)+size);

	msg->write = msg->data;
	msg->read = msg->data;
	msg->data_len = size;

	return msg;
}

static inline void msg_free(msg_t *msg)
{
	btctl_mem_free(msg);
}

/* Return number of bytes written in the data buffer. */
static inline int msg_write_len(const msg_t *msg)
{
	return (int)(msg->write - msg->data);
}

/* Return number of bytes we can pull between
 * the data pointer and the end of the buffer. */
static inline int msg_read_avail(const msg_t *msg)
{
	return (int)(msg->write - msg->read);
}

/* Return number of bytes we can put between
 * the tail pointer and the end of the buffer. */
static inline int msg_write_avail(const msg_t *msg)
{
        return msg->data_len - msg_write_len(msg);
}

/* Set write pointer.
 * Meant to trim the msg to the end of data written. */
static inline int msg_set_write(msg_t *msg, uint8_t *write)
{
	if (write < msg->data || write > (msg->data + msg->data_len))
                return -1;
        msg->write = write;

        return 0;
}

static inline uint8_t *msg_put(msg_t *msg, unsigned int len)
{
        uint8_t *tmp = msg->write;

        if (msg_write_avail(msg) < (int) len)
		DIE("msg_put: short buf\n");
        msg->write += len;

        return tmp;
}

static inline uint8_t *msg_pull(msg_t *msg, unsigned int len)
{
	uint8_t *read = msg->read;
	if (msg_read_avail(msg) < (int) len)
		DIE("msg_pull: short buf\n");
	msg->read += len;
	return read;
}
#endif
