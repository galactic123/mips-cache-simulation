
#include "cache_data.h"
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
Cache_data cache_data;
int waiting_data = 0;
void data_init(){
    int i =0, j = 0;
    for(i=0;i<256;i++)
    {
        for(j=0;j<8;j++)
        {
            cache_data.cache_data_set[i].cache_data_line[j].valid = 0;
            cache_data.cache_data_set[i].cache_data_line[j].iru = 0;
        }
    }
}
uint32_t cache_data_read(uint32_t address)
{
//    printf("read address = 0x%08x\n",address);
    uint32_t line = address >> 13 ;//取前19位
    uint32_t set = (address >>5) &(0xFF);//取所对应的组号
    uint32_t inneraddress = address & 0x1f;
    uint32_t i = 0;
//    printf("line = %d set = %d inneraddress = %d\n",line,set,inneraddress);
    for(i=0;i<8;i++)
    {
        if(cache_data.cache_data_set[set].cache_data_line[i].valid==1)//当前位有效的时候
        {
            if(cache_data.cache_data_set[set].cache_data_line[i].line == line)//当前行号和对应的主存行号相同
            {
                uint32_t j = inneraddress;//内部地址
                uint32_t word = (cache_data.cache_data_set[set].cache_data_line[i].data.mem[j]<<0)    |
                                (cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+1]<<8)  |
                                (cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+2]<<16) |
                                (cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+3]<<24);  
                return word;
                
            }

        }
    }
    waiting_data = 49;
    cache_data_write(address);
    return 0xffffffff;
}
void cache_data_write(uint32_t address)//给定地址，要求把地址所在的块中读出一个块存到cache里
{
    //内存地址 = 行号19位 + 组号8位 + 块内地址5位
    
    uint32_t line = address >> 13 ;//取前19位，要写入的line
    uint32_t set = (address >>5) &(0xFF);//取所对应的组号
    uint32_t begin = (address >> 5);//块头
    begin = begin << 5;
    int i = 0;
    int tobeset = -1;//需要被替换的way
    uint32_t getline = 0;
    uint32_t getset = 0;
    for(i=0;i<8;i++)
    {
        if(cache_data.cache_data_set[set].cache_data_line[i].valid==0)
        {
            cache_data.cache_data_set[set].cache_data_line[i].valid = 1;
            cache_data.cache_data_set[set].cache_data_line[i].line = line;//对应第几个群

            tobeset = i;//要写的列
            
            cache_data.cache_data_set[set].cache_data_line[i].iru = 0;
            break;
        }
    }

    
    for(i=0;i<8;i++)
    {
         if(tobeset==i)continue;
         else
            {
                if(cache_data.cache_data_set[set].cache_data_line[i].iru<7)
                cache_data.cache_data_set[set].cache_data_line[i].iru++;
            }
    }
    if(tobeset==-1){
        uint8_t maxn = 0;
        int i;
        for(i=0;i<8;i++)
        {
            if(cache_data.cache_data_set[set].cache_data_line[i].iru==0)continue;
            else
            {
                if(cache_data.cache_data_set[set].cache_data_line[i].iru<7)
                    cache_data.cache_data_set[set].cache_data_line[i].iru++;
                if(maxn<=cache_data.cache_data_set[set].cache_data_line[i].iru)
                {
                    maxn = cache_data.cache_data_set[set].cache_data_line[i].iru;
                    tobeset = i;//将要替换的行
                    getline = cache_data.cache_data_set[set].cache_data_line[i].line;//将要替换的行的行号
                    getset = set;
                }
            }
        }
        //准备写回主存
        uint32_t readdress = 0;
        readdress = (getline<<13)|(set<<5);//将要替换的行的源地址块头   
        for(i=0;i<8;i++)//一个块是32 byte 要读八次
        {
            uint32_t word = cache_data_read(readdress);//根据主存地址从cache中读出一个字
            mem_write_32(readdress,word);//将这一个字写回到主存
            readdress+=4;
        }
        cache_data.cache_data_set[set].cache_data_line[tobeset].valid = 1;
        cache_data.cache_data_set[set].cache_data_line[tobeset].line = line;//对应第几个群
        cache_data.cache_data_set[set].cache_data_line[tobeset].iru = 0;        
    }
    
    for(i = 0;i<8;i++)//读取32个字节作为一块
    {
        uint32_t word = mem_read_32(begin);
        cache_data.cache_data_set[set].cache_data_line[tobeset].data.mem[i*4]   = (uint8_t)(word>>0)&0xff;
        cache_data.cache_data_set[set].cache_data_line[tobeset].data.mem[i*4+1] = (uint8_t)(word>>8)&0xff;
        cache_data.cache_data_set[set].cache_data_line[tobeset].data.mem[i*4+2] = (uint8_t)(word>>16)&0xff;
        cache_data.cache_data_set[set].cache_data_line[tobeset].data.mem[i*4+3] = (uint8_t)(word>>24)&0xff;
        begin+=4;
    }
}


void cache_data_write_val(uint32_t address,uint32_t write)//把本应写在主存address里的write写到cache里
{
    if(cache_data_read(address)==0xffffffff){
        cache_data_write(address);
    }
    uint32_t line = address >> 13 ;//取前19位
    uint32_t set = (address >>5) &(0xFF);//取所对应的组号
    uint32_t inneraddress = address & 0x1f;
    uint32_t i = 0;
    for(i=0;i<8;i++)
    {
        if(cache_data.cache_data_set[set].cache_data_line[i].valid==1)//当前位有效的时候
        {
            if(cache_data.cache_data_set[set].cache_data_line[i].line == line)//当前行号和对应的主存行号相同
            {
                uint32_t j = inneraddress;//内部地址
                cache_data.cache_data_set[set].cache_data_line[i].data.mem[j]   = (uint8_t)(write>>0)&0xff;
                cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+1] = (uint8_t)(write>>8)&0xff;
                cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+2] = (uint8_t)(write>>16)&0xff;
                cache_data.cache_data_set[set].cache_data_line[i].data.mem[j+3] = (uint8_t)(write>>24)&0xff;
                return;           
            }
        }
    }
}
