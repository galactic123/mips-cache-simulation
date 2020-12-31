#ifndef CACHEDATA
#define CACHEDATA

#include "stdio.h"
#include "mips.h"
#include "stdbool.h"
#include "stdint.h"
typedef struct //数据区，相当于一个块
{
    uint8_t mem[32];//一个块有32byte
}Data_1;
typedef struct //每行cache
{
    bool valid;//是否可用
    bool dirty;
    uint8_t iru;//lru位,因为8way，所以取后三位即可
    uint32_t line;//19位行号    
    Data_1 data;//数据区
}Cache_data_line;
typedef struct //每组cache
{
    Cache_data_line cache_data_line[8];//一组有八行
}Cache_data_set;
typedef struct 
{
    Cache_data_set cache_data_set[256];//一共有256组
}Cache_data;
extern Cache_data cache_data;
void data_init();
void cache_data_write_val(uint32_t address,uint32_t write);//写入一个块
void cache_data_load(uint32_t address);//写入一个块
uint32_t cache_data_read(uint32_t address);//从给定本应读取的内存地址中读取数据
extern int waiting_data;
#endif

