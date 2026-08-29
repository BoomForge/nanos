#include <kernel/heap.h>
#include <kernel/pmm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define HEAP_MAGIC 0x48454150u
#define HEAP_BLOCK_FREE 1u
#define HEAP_BLOCK_USED 0u
#define HEAP_MIN_SPLIT 16u

struct heap_block {
    uint32_t magic;
    uint32_t size;
    uint32_t free;
    struct heap_block *next;
    struct heap_block *prev;
};

static uint8_t *heap_start;
static uint8_t *heap_end;
static struct heap_block *heap_first;

static uint32_t align4(uint32_t value)
{
    return (value + 3u) & ~3u;
}

void heap_init(uint32_t initial_pages)
{
    uint32_t addr;
    uint32_t bytes;
    uint32_t payload_bytes;

    if (initial_pages == 0u) {
        initial_pages = 1u;
    }

    addr = pmm_alloc_pages(initial_pages);
    if (addr == 0u) {
        print_writeln("heap allocation failed");
        return;
    }

    bytes = initial_pages * PMM_PAGE_SIZE;
    heap_start = (uint8_t *)(uintptr_t)addr;
    heap_end = heap_start + bytes;
    memset(heap_start, 0, bytes);

    payload_bytes = bytes - sizeof(struct heap_block);
    heap_first = (struct heap_block *)(void *)heap_start;
    heap_first->magic = HEAP_MAGIC;
    heap_first->size = payload_bytes;
    heap_first->free = HEAP_BLOCK_FREE;
    heap_first->next = NULL;
    heap_first->prev = NULL;

    print_write("heap start ");
    print_hex32(addr);
    print_write(" size ");
    print_uint(bytes);
    print_putc('\n');
}

static void split_block(struct heap_block *block, uint32_t size)
{
    struct heap_block *new_block;
    uint32_t remaining;
    uint8_t *new_addr;

    if (block->size < size + sizeof(struct heap_block) + HEAP_MIN_SPLIT) {
        return;
    }

    remaining = block->size - size - sizeof(struct heap_block);
    new_addr = ((uint8_t *)(void *)block) + sizeof(struct heap_block) + size;
    new_block = (struct heap_block *)(void *)new_addr;

    new_block->magic = HEAP_MAGIC;
    new_block->size = remaining;
    new_block->free = HEAP_BLOCK_FREE;
    new_block->next = block->next;
    new_block->prev = block;

    if (new_block->next != NULL) {
        new_block->next->prev = new_block;
    }

    block->size = size;
    block->next = new_block;
}

static void coalesce_with_next(struct heap_block *block)
{
    struct heap_block *next;

    next = block->next;
    if (next == NULL || next->magic != HEAP_MAGIC || next->free != HEAP_BLOCK_FREE) {
        return;
    }

    block->size += sizeof(struct heap_block) + next->size;
    block->next = next->next;
    if (block->next != NULL) {
        block->next->prev = block;
    }
}

void *heap_alloc(size_t size)
{
    struct heap_block *block;
    uint32_t wanted;

    if (heap_start == NULL || size == 0u) {
        return NULL;
    }

    wanted = align4((uint32_t)size);
    block = heap_first;
    while (block != NULL) {
        if (block->magic == HEAP_MAGIC && block->free == HEAP_BLOCK_FREE && block->size >= wanted) {
            split_block(block, wanted);
            block->free = HEAP_BLOCK_USED;
            memset(((uint8_t *)(void *)block) + sizeof(struct heap_block), 0, block->size);
            return ((uint8_t *)(void *)block) + sizeof(struct heap_block);
        }
        block = block->next;
    }

    return NULL;
}

void heap_free(void *ptr)
{
    struct heap_block *block;

    if (ptr == NULL) {
        return;
    }

    block = (struct heap_block *)(void *)((uint8_t *)ptr - sizeof(struct heap_block));
    if (block->magic != HEAP_MAGIC) {
        print_writeln("heap_free ignored bad pointer");
        return;
    }

    block->free = HEAP_BLOCK_FREE;
    coalesce_with_next(block);

    if (block->prev != NULL && block->prev->free == HEAP_BLOCK_FREE) {
        coalesce_with_next(block->prev);
    }
}

struct heap_stats heap_get_stats(void)
{
    struct heap_block *block;
    struct heap_stats stats;

    stats.total_bytes = 0u;
    stats.used_bytes = 0u;
    stats.free_bytes = 0u;
    stats.blocks = 0u;

    if (heap_start == NULL) {
        return stats;
    }

    stats.total_bytes = (uint32_t)(uintptr_t)heap_end - (uint32_t)(uintptr_t)heap_start;

    block = heap_first;
    while (block != NULL && block->magic == HEAP_MAGIC) {
        ++stats.blocks;
        if (block->free == HEAP_BLOCK_FREE) {
            stats.free_bytes += block->size;
        } else {
            stats.used_bytes += block->size;
        }
        block = block->next;
    }

    return stats;
}

void heap_print_summary(void)
{
    struct heap_stats stats;

    stats = heap_get_stats();
    if (stats.total_bytes == 0u) {
        print_writeln("heap not ready");
        return;
    }

    print_write("heap used ");
    print_uint(stats.used_bytes);
    print_write("/");
    print_uint(stats.total_bytes);
    print_write(" free ");
    print_uint(stats.free_bytes);
    print_write(" blocks ");
    print_uint(stats.blocks);
    print_putc('\n');
}
