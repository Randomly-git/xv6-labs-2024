// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define BATCH_SIZE 8

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char name[8];
} kmem[NCPU];

void
kinit()
{
  for(int i=0;i<NCPU;i++){
    snprintf(kmem[i].name,6,"kmem%d",i);
    initlock(&kmem[i].lock, kmem[i].name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int cpu_id = cpuid();
  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r, *stolen_head, *stolen_tail;
  push_off();
  int cpu_id = cpuid();
  
  // 尝试本地分配
  acquire(&kmem[cpu_id].lock);
  if((r = kmem[cpu_id].freelist)) {
    kmem[cpu_id].freelist = r->next;
    release(&kmem[cpu_id].lock);
    pop_off();
    if(r) memset((char*)r, 5, PGSIZE);
    return (void*)r;
  }
  release(&kmem[cpu_id].lock);

  // 从其他CPU盗取
  for(int i = 0; i < NCPU; i++) {
    if(i == cpu_id) continue;
    
    acquire(&kmem[i].lock);
    if(kmem[i].freelist) {
      // 盗取一批页面
      stolen_head = kmem[i].freelist;
      stolen_tail = stolen_head;
      
      // 找到这批页面的末尾
      for(int count = 1; count < BATCH_SIZE && stolen_tail->next; count++) {
        stolen_tail = stolen_tail->next;
      }
      
      // 从源CPU分离
      kmem[i].freelist = stolen_tail->next;
      stolen_tail->next = 0;
      release(&kmem[i].lock);
      
      // 将盗取的页面加入本地列表
      acquire(&kmem[cpu_id].lock);
      stolen_tail->next = kmem[cpu_id].freelist;
      kmem[cpu_id].freelist = stolen_head->next; 
      r = stolen_head; 
      release(&kmem[cpu_id].lock);
      
      pop_off();
      if(r) memset((char*)r, 5, PGSIZE);
      return (void*)r;
    }
    release(&kmem[i].lock);
  }
  
  pop_off();
  return 0;
}