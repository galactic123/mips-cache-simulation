#ifndef CACHE
#define CACHE

#include "stdio.h"
#include "mips.h"
#include "stdbool.h"
#include "stdint.h"
typedef struct //数据区，相当于一个块
{
    uint8_t mem[32];//一个块有32byte
}Data;

typedef struct //每行cache
{
    bool valid;//是否可用
    uint8_t iru;//iru位,因为4way，所以取后两位即可
    uint16_t line;//9位行号    
    Data data;//数据区
}Cache_line;

typedef struct //每组cache
{
    Cache_line cache_line[4];//一组有四行
}Cache_set;
typedef struct 
{
    Cache_set cache_set[64];//一共有64组
}Cache;
extern Cache cache;
void init();
void get_set(uint32_t address);
void cache_write(uint32_t address);//写入一个块
uint32_t cache_read(uint32_t address);//从给定本应读取的内存地址中读取数据
void cache_replace(Cache_line cache_line);//给定行，进行替换

extern int waiting;

#endif