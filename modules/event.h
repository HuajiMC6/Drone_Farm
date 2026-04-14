#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum
{
    EVENT_FIELD_UPDATE,
    EVENT_PLAYER_UPDATE,
} event_type_t;

typedef struct
{
    event_type_t type;
    void *data;
} event_t;

void event_send(event_type_t type, void *data);
event_t *event_get();

#endif