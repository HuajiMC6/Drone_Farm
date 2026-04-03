#include "enum.h"
#include "farm.h"
#include "stdlib.h"

farm_t* farm_init(){
    farm_t* farm=(farm_t*)malloc(sizeof(farm_t));
    for(int i=0;i<10;i++) for(int j=0;j<10;j++) farm->fields[i][j]=field_init();
    farm->current_size=5;
    farm->size_level=0;
    return farm;
}

void size_update(farm_t* farm){
    if(farm->size_level>=3) return;
    farm->size_level++;
    if(farm->size_level==0) farm->current_size=5;
    else if(farm->size_level==1) farm->current_size=7;
    else if(farm->size_level==2) farm->current_size=9;
    else if(farm->size_level==3) farm->current_size=10;
}

void farm_grow(farm_t* farm){
    for(int i=0;i<farm->current_size;i++) for(int j=0;j<farm->current_size;j++) if(farm->fields[i][j]->crop_type!=CROP_TYPE_NONE) field_grow(farm->fields[i][j]);
}

int get_farm_size(farm_t* farm){
    return farm->current_size;
}