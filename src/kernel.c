#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_column = 0;

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_put_char(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_write_char(char c, char colour)
{
    if (c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
        return;
    }

    terminal_put_char(terminal_column, terminal_row, c, colour);
    terminal_column++;
    if (terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }

}

void terminal_init()
{
    video_mem = (uint16_t*)(0xB8000);

    terminal_row = 0;
    terminal_column = 0;

    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_put_char(x, y, ' ', 0);
        }
    }
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while(str[len])
    {
        len++;
    }
    return len;
}

void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_write_char(str[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;
void kernel_main()
{
    terminal_init();
    print("Hello World!\n");

    kheap_init();
    idt_init();
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    char* ptr = kzalloc(4096);
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);
    enable_paging();

    char* ptr2 = (char*) 0x1000;
    ptr2[0] = 'A';
    ptr2[1] = 'B';
    print(ptr2);
    print(ptr);

    enable_interrupts();
}
