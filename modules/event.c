#include "event.h"

#define EVENT_QUEUE_SIZE 10

static event_t event_sequence[EVENT_QUEUE_SIZE];
static unsigned int event_head = 0;
static unsigned int event_tail = 0;
static unsigned int event_count = 0;
static event_t event_out;

void event_send(event_type_t type, void *data) {
    event_sequence[event_tail].type = type;
    event_sequence[event_tail].data = data;

    event_tail = (event_tail + 1U) % EVENT_QUEUE_SIZE;

    if (event_count >= EVENT_QUEUE_SIZE) {
        event_head = (event_head + 1U) % EVENT_QUEUE_SIZE;
    } else {
        event_count++;
    }
}

event_t *event_get() {
    if (event_count == 0U) {
        return 0;
    }

    event_out = event_sequence[event_head];
    event_head = (event_head + 1U) % EVENT_QUEUE_SIZE;
    event_count--;

    return &event_out;
}