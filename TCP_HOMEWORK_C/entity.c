/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names:
// Student computing IDs:
//
//
// This file contains the actual code for the functions that will implement the
// reliable transport protocols enabling entity "A" to reliably send information
// to entity "B".
//
// This is where you should write your code, and you should submit a modified
// version of this file.
//
// Notes:
// - One way network delay averages five time units (longer if there are other
//   messages in the channel for GBN), but can be larger.
// - Packets can be corrupted (either the header or the data portion) or lost,
//   according to user-defined probabilities entered as command line arguments.
// - Packets will be delivered in the order in which they were sent (although
//   some can be lost).
// - You may have global state in this file, BUT THAT GLOBAL STATE MUST NOT BE
//   SHARED BETWEEN THE TWO ENTITIES' FUNCTIONS. "A" and "B" are simulating two
//   entities connected by a network, and as such they cannot access each
//   other's variables and global state. Entity "A" can access its own state,
//   and entity "B" can access its own state, but anything shared between the
//   two must be passed in a `pkt` across the simulated network. Violating this
//   requirement will result in a very low score for this project (or a 0).
//
// To run this project you should be able to compile it with something like:
//
//     $ gcc entity.c simulator.c -o myproject
//
// and then run it like:
//
//     $ ./myproject 0.0 0.0 10 500 3 test1.txt
//
// Of course, that will cause the channel to be perfect, so you should test
// with a less ideal channel, and you should vary the random seed. However, for
// testing it can be helpful to keep the seed constant.
//
// The simulator will write the received data on entity "B" to a file called
// `output.dat`.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simulator.h"


#define BUFSIZE 1000

struct Sender
{
    int base;
    int nextseq;
    int window_size;
    float estimated_rtt;
    int buffer_next;
    struct pkt packet_buffer[BUFSIZE];
} A;

struct Receiver
{
    int expect_seq;
    struct pkt packet_to_send;
} B;

int get_checksum(struct pkt *packet)
{
    int checksum = 0;
    checksum += packet->seqnum;
    checksum += packet->acknum;
    for (int i = 0; i < 20; ++i)
        checksum += packet->payload[i];
    return checksum;
}

void send_window(void)
{
    while (A.nextseq < A.buffer_next && A.nextseq < A.base + A.window_size)
    {
        struct pkt *packet = &A.packet_buffer[A.nextseq % BUFSIZE];
        tolayer3_A(*packet);
        if (A.base == A.nextseq)
            starttimer_A(A.estimated_rtt);
        ++A.nextseq;
    }
}

/**** A ENTITY ****/


void A_init() {
    A.base = 1;
    A.nextseq = 1;
    A.window_size = 8;
    A.estimated_rtt = 15;
    A.buffer_next = 1;
}

void A_output(struct msg message) {
    if (A.buffer_next - A.base >= BUFSIZE)
    {
        return;
    }
    struct pkt *packet = &A.packet_buffer[A.buffer_next % BUFSIZE];
    packet->seqnum = A.buffer_next;
    memmove(packet->payload, message.data, message.length);
    packet->checksum = get_checksum(packet);
    ++A.buffer_next;
    send_window();
}

void A_input(struct pkt packet) {
  if (packet.checksum != get_checksum(&packet))
    {
        return;
    }
    if (packet.acknum < A.base)
    {
        return;
    }
    A.base = packet.acknum + 1;
    if (A.base == A.nextseq)
    {
        stoptimer_A();
        send_window();
    }
    else
    {
        starttimer_A(A.estimated_rtt);
    }
}

void A_timerinterrupt() {
    for (int i = A.base; i < A.nextseq; ++i)
    {
        struct pkt *packet = &A.packet_buffer[i % BUFSIZE];
        tolayer3_A(*packet);
    }
    starttimer_A(A.estimated_rtt);
}


/**** B ENTITY ****/

void B_init() {
    B.expect_seq = 1;
    B.packet_to_send.seqnum = -1;
    B.packet_to_send.acknum = 0;
    memset(B.packet_to_send.payload, 0, 20);
    B.packet_to_send.checksum = get_checksum(&B.packet_to_send);
}

void B_input(struct pkt packet) {
    if (packet.checksum != get_checksum(&packet))
    {
        tolayer3_B(B.packet_to_send);
        return;
    }
    if (packet.seqnum != B.expect_seq)
    {
        tolayer3_B(B.packet_to_send);
        return;
    }

    struct msg message;
    message.length = strlen(packet.payload);
    strncpy(message.data, packet.payload, 20);
    tolayer5_B(message);

    B.packet_to_send.acknum = B.expect_seq;
    B.packet_to_send.checksum = get_checksum(&B.packet_to_send);
    tolayer3_B(B.packet_to_send);

    ++B.expect_seq;
}

void B_timerinterrupt() {
}
