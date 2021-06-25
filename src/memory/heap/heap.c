#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"

static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    int response = 0;

    size_t table_size = (size_t)(end-ptr);
    size_t total_blocks = table_size / HEAP_ENTRY_SIZE;
    if (table->total != total_blocks)
    {
        response = -EINVARG;
        goto out;
    }

out:
    return response;
}

static int heap_validate_alignment(void* ptr)
{
    return ((unsigned int)ptr % HEAP_ENTRY_SIZE) == 0;
}

int heap_init(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int response = 0;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        response = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->start_addr = ptr;
    heap->table = table;

    response  = heap_validate_table(ptr, end, table);
    if (response < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_ENTRY_FREE, table_size);

out:
    return response;
}

static uint32_t heap_align_value_upper(uint32_t val)
{
    if ((val % HEAP_ENTRY_SIZE) == 0) 
    {
        return val;
    }

    val = val - (val % HEAP_ENTRY_SIZE) + HEAP_ENTRY_SIZE;
    return val;

}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0F;
}

int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int block_current = 0;
    int block_start = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_ENTRY_FREE)
        {
            block_current = 0;
            block_start = -1;
            continue;
        }

        if (block_start == -1)
        {
            block_start = i;
        }
        block_current++;

        if (block_current == total_blocks)
        {
            break;
        }
    }

    if (block_start == -1)
    {
        return -ENOMEM;
    }

    return block_start;
}

void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->start_addr + (block * HEAP_ENTRY_SIZE);
}

void mark_taken(struct heap* heap, int start, int total_blocks)
{
    int end = (start + total_blocks) - 1;
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_ENTRY_TAKEN | HEAP_BLOCK_FIRST;
    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start; i <= end; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_ENTRY_TAKEN;
        if (i != end - 1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;
    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);
    mark_taken(heap, start_block, total_blocks);

out:
    return address;

}

int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address - heap->start_addr)) / HEAP_ENTRY_SIZE;
}

void heap_mark_blocks_free(struct heap* heap, int start)
{
    struct heap_table* table = heap->table;
    for (int i = start; i < (int) table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_ENTRY_FREE;
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_upper(size);
    uint32_t total_blocks = aligned_size / HEAP_ENTRY_SIZE;

    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap* heap, void* ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}