
#include "cache.h"
#include "pipe.h"
#include "shell.h"
#include "mips.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MEM_DATA_START  0x10000000
#define MEM_DATA_SIZE   0x00100000
#define MEM_TEXT_START  0x00400000
#define MEM_TEXT_SIZE   0x00100000
#define MEM_STACK_START 0x7ff00000
#define MEM_STACK_SIZE  0x00100000
#define MEM_KDATA_START 0x90000000
#define MEM_KDATA_SIZE  0x00100000
#define MEM_KTEXT_START 0x80000000
#define MEM_KTEXT_SIZE  0x00100000
Cache cache;

int waiting = 0;

void init(){
    int i =0, j = 0;
    for(i=0;i<64;i++)
    {
        for(j=0;j<4;j++)
        {
            cache.cache_set[i].cache_line[j].valid = 0;
            cache.cache_set[i].cache_line[j].iru = 0;
        }
    }
}

void get_set(uint32_t address)
{
    uint32_t tmp = address & 0xFFFFF;//取后20位
    uint32_t line = tmp >> 11 ;//取12-20位
    uint32_t set = (tmp >>5) &(0x6F);//取所对应的组号
    uint32_t begin = (address >> 5)<<5;
    printf("%d %d %d\n",tmp,line,set);
}
uint32_t cache_read(uint32_t address)
{
    uint32_t tmp = address & 0xFFFFF;//取后20位
    uint32_t line = tmp >> 11 ;//取12-20位
    uint32_t set = (tmp >>5) &(0x3F);//取所对应的组号
    uint32_t inneraddress = address & 0x1f;
//    printf("inneraddress = %08x\n",inneraddress);
    uint32_t i = 0;
//    printf("ok_read\n");
    for(i=0;i<4;i++)
    {
        if(cache.cache_set[set].cache_line[i].valid==1)
        {
            if(cache.cache_set[set].cache_line[i].line == line)
            {
                uint32_t j = inneraddress;
                uint32_t word = (cache.cache_set[set].cache_line[i].data.mem[j]<<0)  |
                                (cache.cache_set[set].cache_line[i].data.mem[j+1]<<8)|
                                (cache.cache_set[set].cache_line[i].data.mem[j+2]<<16) |
                                (cache.cache_set[set].cache_line[i].data.mem[j+3]<<24);  
 //                               printf("hit!");
                                //system("pause");
                return word;
                
            }

        }
    }
    //printf("nohit!");
    waiting = 49;
    cache_write(address);
    return 0xffffffff;
}
void cache_write(uint32_t address)//写入一个块
{
//    printf("oknow\n");
    uint32_t tmp = address & 0xFFFFF;//取后20位
    uint32_t line = tmp >> 11 ;//取12-20位
    uint32_t set = (tmp >>5) &(0x3F);//取所对应的组号
    uint32_t begin = (address >> 5);//块头
//    printf("set = %08x\n",set);
    begin = begin << 5;
    int i = 0;
    int tobeset = -1;
    
    for(i=0;i<4;i++)
    {
        if(cache.cache_set[set].cache_line[i].valid==0)
        {
            cache.cache_set[set].cache_line[i].valid = 1;
            cache.cache_set[set].cache_line[i].line = line;//对应第几个群
//            printf("%d %d\n",set,i);
            tobeset = i;//要写的列
            cache.cache_set[set].cache_line[i].iru = 0;
            break;
        }
    }
    for(i=0;i<4;i++)
    {
         if(tobeset==i)continue;
         else
            {
                if(cache.cache_set[set].cache_line[i].iru<3)
                cache.cache_set[set].cache_line[i].iru++;
            }
    }
    if(tobeset==-1){
        uint8_t maxn = 0;
        int i;
        for(i=0;i<4;i++)
        {
            if(cache.cache_set[set].cache_line[i].iru==0)continue;
            else
            {
                if(cache.cache_set[set].cache_line[i].iru<3)
                    cache.cache_set[set].cache_line[i].iru++;
                if(maxn<=cache.cache_set[set].cache_line[i].iru)
                {
                    maxn = cache.cache_set[set].cache_line[i].iru;
                    tobeset = i;
                }
            }
        }
        // if(tobeset < 0 || tobeset > 3) {
        //     printf("%d\n",tobeset);
        //     printf("fuck you re!");
        //     exit(1);
        // }
        cache.cache_set[set].cache_line[tobeset].valid = 1;
        cache.cache_set[set].cache_line[tobeset].line = line;//对应第几个群
        cache.cache_set[set].cache_line[tobeset].iru = 0;        
 //       return;//待修改
    }
    
 //   printf("%08x",begin);
    for(i = 0;i<8;i++)//读取32个字节作为一块
    {
        uint32_t word = mem_read_32(begin);
        cache.cache_set[set].cache_line[tobeset].data.mem[i*4]   = (uint8_t)(word>>0)&0xff;
        cache.cache_set[set].cache_line[tobeset].data.mem[i*4+1] = (uint8_t)(word>>8)&0xff;
        cache.cache_set[set].cache_line[tobeset].data.mem[i*4+2] = (uint8_t)(word>>16)&0xff;
        cache.cache_set[set].cache_line[tobeset].data.mem[i*4+3] = (uint8_t)(word>>24)&0xff;
        begin+=4;
    }
    
}