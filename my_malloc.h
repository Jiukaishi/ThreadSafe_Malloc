#ifndef MY_MALLOC_H
#define MY_MALLOC_H
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
struct block_t {
  size_t sz;
  struct block_t *next;
  struct block_t *prev;
};
typedef struct block_t block;
struct block_list_t {
  block *head;
  block *tail;
  size_t sz;
};
typedef struct block_list_t block_list;

block_list *blist = NULL;
__thread block_list *th_blist = NULL;
pthread_mutex_t lk = PTHREAD_MUTEX_INITIALIZER;
void *create_block(size_t size);
void *create_block_nolock(size_t size);
int split_block(size_t size, block *b);
int split_block_nolock(size_t size, block *b);
void merge_free_block(block *, block *);
void merge_free_block_nolock(block *prev_block, block *cur_block);
void add_to_list(block *);
void add_to_list_nolock(block *b);
void ts_free_lock(void *ptr);
void ts_free_nolock(void *ptr);
void *ts_malloc_lock(size_t size);
void *ts_malloc_nolock(size_t size);
unsigned long get_data_segment_size();
long long get_data_segment_free_space_size();
unsigned long total_size = 0;
long long free_size = 0;
#endif
