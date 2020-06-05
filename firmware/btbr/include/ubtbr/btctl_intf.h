#ifndef __BTCTL_INTF_H
#define __BTCTL_INTF_H
#include <ubtbr/bb_msg.h>
#include <ubtbr/queue.h>
#include <ubtbr/system.h>
#include <ubertooth_interface.h>

typedef struct btctl_s {
	btctl_state_t state;
	msg_queue_t rx_q;
	msg_queue_t tx_q;
	msg_queue_t acl_tx_q;
	msg_t *eir_msg;
	btctl_tx_pkt_t *eir_pkt;
} btctl_t;

extern btctl_t btctl;

void btctl_init(void);
int btctl_work(void);

void btctl_set_state(btctl_state_t state, btctl_reason_t reason);

static inline btctl_state_t btctl_get_state(void)
{
	return btctl.state;
}

static inline btctl_tx_pkt_t *btctl_get_eir(void)
{
	return btctl.eir_pkt;
}

static inline msg_t *btctl_msg_alloc(unsigned type)
{
	msg_t *msg;
	btctl_hdr_t *hdr;

	/* maximum size ?*/
	msg = msg_alloc(sizeof(btctl_hdr_t)+sizeof(btctl_rx_pkt_t)+MAX_ACL_PACKET_SIZE);
	hdr = (btctl_hdr_t*) msg_put(msg, sizeof(*hdr));
	hdr->type = type;

	return msg;
}

static inline msg_t *safe_dequeue(msg_queue_t* q)
{
	uint32_t flags = irq_save_disable();
	msg_t *msg;

	msg = msg_dequeue(q);
	irq_restore(flags);

	return msg;
}

static inline int safe_enqueue(msg_queue_t *q, msg_t *msg)
{
	uint32_t flags = irq_save_disable();
	int rc;

	rc = msg_enqueue(q, msg);
	irq_restore(flags);

	return rc;
}

/* Called by usb driver to retreive next message to send to host */
static inline msg_t *btctl_tx_dequeue(void)
{
	return safe_dequeue(&btctl.tx_q);
}

/* Enqueue a message for host */
static inline int btctl_tx_enqueue(msg_t *msg)
{
	return safe_enqueue(&btctl.tx_q, msg);
}

/* Dequeue a message from host */
static inline msg_t *btctl_rx_dequeue(void)
{
	return safe_dequeue(&btctl.rx_q);
}

/* Called by usb driver with last message received from host */
static inline int btctl_rx_enqueue(msg_t *msg)
{
	return safe_enqueue(&btctl.rx_q, msg);
}

/* Called by connected task to retreive next packet to send */
static inline msg_t *btctl_acl_tx_dequeue(void)
{
	return safe_dequeue(&btctl.acl_tx_q);
}

/* Enqueue a packet to send in connected state */
static inline int btctl_acl_tx_enqueue(msg_t *msg)
{
	return safe_enqueue(&btctl.acl_tx_q, msg);
}
#endif
