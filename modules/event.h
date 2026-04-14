#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum
{
    EVENT_ON_FIELD_PLANTED,            // 种植事件
    EVENT_ON_FIELD_CLEARED,            // 清空事件
    EVENT_ON_FIELD_HARVESTED,          // 收获事件
    EVENT_ON_CROP_STAGE_CHANGE,        // 作物生长阶段改变事件
    EVENT_ON_PEST_DETECTED,            // 虫害检测事件
    EVENT_ON_PEST_COMMITTED,           // 虫害发生事件
    EVENT_ON_PEST_CLEARED,             // 虫害清除事件
    EVENT_ON_FIELD_UPGRADE,            // 田地升级事件
    EVENT_ON_PLAYER_COIN_CHANGE,       // 玩家金币改变事件
    EVENT_ON_PLAYER_EXPERIENCE_CHANGE, // 玩家经验改变事件
    EVENT_ON_PLAYER_LEVEL_UPGRADE,     // 玩家等级提升事件
} event_type_t;

typedef struct
{
    event_type_t type;
    void *data;
} event_t;

void event_send(event_type_t type, void *data);
event_t *event_get();

#endif