#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

#define DISK_TYPE_REAL 0;

typedef unsigned int DISK_TYPE;

struct disk
{
    DISK_TYPE type;
    int sector_size;

    struct filesystem* filesystem;
};

void disk_search_and_init();
struct disk* disk_get(int index);
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buffer);

#endif