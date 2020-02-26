#include "cache_associative_full.h"
#include <stdio.h>
#include <stdbool.h>

struct Line
{
    unsigned char valid;
    unsigned char dirty;
    unsigned int  tag:28;
    unsigned char block[16];
    unsigned int time;
};

struct Full
{
    struct Line line[16];
}cache;

struct MemoryAddress
{
    unsigned char offset:4;
    unsigned int tag:28;
};

int memoryLoads;

int storeCount;

int hits;

int misses;

void cache_associative_full_init()
{
    memoryLoads = 0;
    
    storeCount = 0;
    
    hits = 0;
    
    misses = 0;
    
    for(int i = 0; i < 16; i++)
    {
        cache.line[i].valid = 0;
        cache.line[i].dirty = 0;
    }
}

int cache_associative_full_load(memory_address addr)
{
    unsigned char line[16];
    bool replaced = false;
    int startingHitCount = hits;
    int value = 0;
    short emptyMatchingTagIndex;
    struct MemoryAddress address;
    int earliestTime = 10000;
    short earliestLine = -1;
    address.offset = addr & 15;
    address.tag = (addr >> 8) & 0xFFFFFFF;
    for(int i = 0; i < 16; i++)
    {
        if(cache.line[i].valid == 1)
        {
            if(address.tag == cache.line[i].tag)
            {
                hits++;
                memcpy(&value, &cache.line[i].block[address.offset - 1], 4);
                cache.line[i].time = hits + misses;
                break;
            }
            else
            {
                misses++;
            }
        }
        else
        {
            misses++;
            if(address.tag == cache.line[i].tag)
            {
                replaced = true;
                cache.line[i].time = hits + misses;
                emptyMatchingTagIndex = i;
                storage_load_line(addr, line);
                memcpy(&value, &line[address.offset], 4);
                memcpy(&cache.line[i].block[0], &line[0], 16);
                break;
            }

            
        }
    }
    if(startingHitCount == hits)
    {
        misses++;
        if(!replaced)
        {
            for(int i = 0; i < 16; i++)
            {
                if(cache.line[i].valid == 1)
                {
                    if(earliestTime > cache.line[i].time)
                    {
                        earliestTime = cache.line[i].time;
                        earliestLine = i;
                    }
                }
            }
            cache.line[earliestLine].tag = address.tag;
            cache.line[earliestLine].time = hits + misses;
            storage_load_line(addr, line);
            memcpy(&value, &line[address.offset], 4);
            memcpy(&cache.line[earliestLine].block[0], &line[0], 16);
        }
        
    }
    
    return value;
}

void cache_associative_full_store(memory_address addr, int value)
{
    int currentHitCount = hits;
    int earliestTime = 10000;
    short earliestLine = -1;
    struct MemoryAddress address;
    address.offset = addr & 15;
    address.tag = (addr >> 8) & 0xFFFFFFF;
    for(int i = 0; i < 16; i++)
    {
        if(cache.line[i].tag == address.tag)
        {
            hits++;
            
            cache.line[i].valid = 1;
            memcpy(&cache.line[i].block[address.offset], &value , 4);
            if(cache.line[i].valid == 1)
            {
                cache.line[i].dirty = 1;
                cache_associative_full_flush();
                
            }
            cache.line[i].time = hits + misses;
            break;
        }
        else
        {
            misses++;
        }
    }
    if(currentHitCount == hits)
    {
        for(int i = 0; i < 16; i++)
        {
            if(cache.line[i].valid == 0)
            {
                hits++;
                cache.line[i].dirty = 1;
                memcpy(&cache.line[i].block[address.offset], &value , 4);
                cache.line[i].tag = address.tag;
                cache.line[i].valid = 1;
                cache.line[i].time = hits + misses;
                cache_associative_full_flush();
            }
            else
            {
                misses++;
            }
        }
    }
    if(currentHitCount == hits)
    {
        for(int i = 0; i < 16; i++)
        {
            if(earliestTime > cache.line[i].time)
            {
                earliestTime = cache.line[i].time;
                earliestLine = i;
            }

        }
        hits++;
        cache.line[earliestLine].dirty = 1;
        memcpy(&cache.line[earliestLine].block[address.offset], &value , 4);
        cache.line[earliestLine].tag = address.tag;
        cache_associative_full_flush();
        cache.line[earliestLine].time = hits + misses;
    }
    
    
}

void cache_associative_full_flush()
{
    unsigned int address = 0;
    
    for (int i = 0; i < 16; i++)
    {
        if (cache.line[i].dirty == 1)
        {
            address |= cache.line[i].tag;
            address <<= 4;
            storage_load_line(address, cache.line[i].block);
            address = 0;
            cache.line[i].dirty = 0;
        }
    }
}

void cache_associative_full_stats()
{
    printf("Hits %i\n", hits);
    printf("Misses %i\n", misses);
}
