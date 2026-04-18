#include "enum.h"

static const char *crop_type_names[CROP_TYPE_NONE] = {
    [CROP_TYPE_WHEAT] = "Wheat",
    [CROP_TYPE_RICE] = "Rice",
    [CROP_TYPE_CORN] = "Corn",
};

static const char *crop_pest_names[CROP_DAMAGE_NONE] = {
    [CROP_DAMAGE_APHID] = "Aphid",
    [CROP_DAMAGE_MITE] = "Mite",
    [CROP_DAMAGE_LEAFROLLER] = "Leafroller",
    [CROP_DAMAGE_LOCUST] = "Locust",
};

static const char *crop_pesticide_names[CROP_PESTICIDE_NONE] = {
    [CROP_PESTICIDE_APHICIDE] = "Aphicide",
    [CROP_PESTICIDE_ACARICIDE] = "Acaricide",
    [CROP_PESTICIDE_LEAFROLLERICIDE] = "Leafrollericide",
    [CROP_PESTICIDE_LOCUSTICIDE] = "Locusticide",
};

static const char *crop_stage_names[CROP_STAGE_NONE] = {
    [CROP_STAGE_SEED] = "Seed",   [CROP_STAGE_YOUNG] = "Young", [CROP_STAGE_GROW] = "Grow",
    [CROP_STAGE_BLOOM] = "Bloom", [CROP_STAGE_RIPE] = "Ripe",   [CROP_STAGE_READY] = "Ready",
};

static const char *drone_state_names[DRONE_STATE_NONE] = {
    [DRONE_STATE_FREE] = "Free",
    [DRONE_STATE_DETECTING] = "Detecting",
    [DRONE_STATE_AUTO] = "Auto",
};

const char *crop_type_name(crop_type_t type) {
    if (type >= CROP_TYPE_NONE)
        return "Unknown";
    return crop_type_names[type];
}

const char *crop_pest_name(crop_damage_t pest) {
    if (pest >= CROP_DAMAGE_NONE)
        return "Unknown";
    return crop_pest_names[pest];
}

const char *crop_pesticide_name(crop_pesticide_t pesticide) {
    if (pesticide >= CROP_PESTICIDE_NONE)
        return "Unknown";
    return crop_pesticide_names[pesticide];
}

const char *crop_stage_name(crop_stage_t stage) {
    if (stage >= CROP_STAGE_NONE)
        return "Unknown";
    return crop_stage_names[stage];
}

const char *drone_state_name(drone_state_t state) {
    if (state >= DRONE_STATE_NONE)
        return "Unknown";
    return drone_state_names[state];
}