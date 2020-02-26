#include "cache_direct.h"

#include <stdio.h>

struct Line
{
    unsigned char valid;
    unsigned char dirty;
    unsigned int  tag:24;
    unsigned char block[16];
    unsigned int time;
};

struct Set
{
    struct Line line[2];
};

struct MemoryAddress
{
    unsigned char offset:4;
    unsigned char setIndex:4;
    unsigned int tag:24;
};

struct Direct
{
    struct Line set[16];
}cache;

int memoryLoads;

int storeCount;

int hits;

int misses;

void cache_direct_init()
{
    memoryLoads = 0;
    
    storeCount = 0;
    
    hits = 0;
    
    misses = 0;
    
    for(int i = 0; i < 16; i++)
    {
        cache.set[i].valid = 0;
        cache.set[i].dirty = 0;
    }
}

int cache_direct_load(memory_address addr)
{
    unsigned char line[16];
    int data;
    struct MemoryAddress address;
    address.offset = addr & 15;
    address.setIndex = (addr >> 4) & 15;
    address.tag = (addr >> 8) & 0xFFFFFF;
    if(cache.set[address.setIndex].valid == 1 && cache.set[address.setIndex].tag == address.tag)
    {
        hits++;
        memoryLoads++;
        memcpy(&data, &cache.set[address.setIndex].block[address.offset], 4);
        cache.set[address.setIndex].time = hits + misses;
    }
    else
    {
        storage_load_line(addr, line);
        memcpy(&data, &line[address.offset], 4);
        cache.set[address.setIndex].tag = address.tag;
        memcpy(&cache.set[address.setIndex].block[0], line, 16);
        cache.set[address.setIndex].valid = 1;
        misses++;
        cache.set[address.setIndex].time = hits + misses;
        printf("\nMiss\n");
    }
    return data;
    
}

void cache_direct_store(memory_address addr, int value)
{
    storeCount++;
    struct MemoryAddress address;
    address.offset = addr & 15;
    address.setIndex = (addr >> 4) & 15;
    address.tag = (addr >> 8) & 0xFFFFFF;
    if(cache.set[address.setIndex].tag != address.tag)
    {
        misses++;
        cache.set[address.setIndex].tag = address.tag;
    }
    else hits++;
    cache.set[address.setIndex].dirty = 1;
    cache.set[address.setIndex].valid = 1;
    memcpy(&cache.set[address.setIndex].block[address.offset - 1], &value , 4);
    cache_direct_flush();
    cache.set[address.setIndex].time = hits + misses;
}

void cache_direct_flush()
{
    unsigned int address = 0;
    
    for (int i = 0; i < 16; i++)
    {
        if (cache.set[i].dirty == 1)
        {
            address |= cache.set[i].tag;
            address <<= 4;
            address |= i;
            address <<= 4;
            storage_load_line(address, cache.set[i].block);
            address = 0;
            cache.set[i].dirty = 0;
        }
    }
}

void cache_direct_stats()
{
    printf("Hits %i\n", hits);
    printf("Misses %i\n", misses);
}
