
#include "my_malloc.h"
#include <assert.h>
#include <limits.h>

unsigned long get_data_segment_size() { return total_size; }

long long get_data_segment_free_space_size() { return free_size; }

void init_th_blist() {
  // if the blist(free list) is empty, initialize it.
  if (th_blist == NULL) {
    pthread_mutex_lock(&lk);
    th_blist = sbrk(sizeof(block_list));
    th_blist->head = sbrk(sizeof(block));
    th_blist->tail = sbrk(sizeof(block));
    pthread_mutex_unlock(&lk);
    th_blist->head->prev = NULL;
    th_blist->tail->next = NULL;
    th_blist->sz = 0;
    th_blist->head->next = th_blist->tail;
    th_blist->head->sz = 0;
    th_blist->tail->prev = th_blist->head;
    th_blist->tail->sz = 0;
  }
  return;
}

void init_blist() {
  // if the blist(free list) is empty, initialize it.
  if (blist == NULL) {
    blist = sbrk(sizeof(block_list));
    blist->head = sbrk(sizeof(block));
    blist->tail = sbrk(sizeof(block));
    blist->head->prev = NULL;
    blist->tail->next = NULL;
    blist->sz = 0;
    blist->head->next = blist->tail;
    blist->head->sz = 0;
    blist->tail->prev = blist->head;
    blist->tail->sz = 0;
  }
  return;
}

void *create_block(size_t size) {

  void *p = sbrk(size + sizeof(block)); // memory for meta and data
  if (p == (void *)-1) {
    perror("create block failed\n");
    exit(EXIT_FAILURE);
  }
  block *new_block = p;
  new_block->sz = size;
  new_block->prev = NULL;
  new_block->next = NULL;
  total_size += size + sizeof(block);
  return new_block + 1;
}
void *create_block_nolock(size_t size) {
  pthread_mutex_lock(&lk);
  void *p = sbrk(size + sizeof(block)); // memory for meta and data
  if (p == (void *)-1) {
    perror("create block failed\n");
    pthread_mutex_unlock(&lk);
    exit(EXIT_FAILURE);
  }

  pthread_mutex_unlock(&lk);
  block *new_block = p;
  new_block->sz = size;
  new_block->prev = NULL;
  new_block->next = NULL;
  total_size += size + sizeof(block);
  return new_block + 1;
}

int split_block(size_t size, block *b) {
  if (size > b->sz) {
    return 0; // if the left memory is not enough for second
  }
  if (size + sizeof(block) > b->sz) {

    blist->sz--;
    b->prev->next = b->next;
    b->next->prev = b->prev;
    b->next = NULL;
    b->prev = NULL;
    free_size -= sizeof(block) + b->sz;
    return 1; // if the left memory is not enough for second
  }
  // printf("reuse and divide\n");
  free_size -= sizeof(block) + size;
  block *new_b = (block *)((char *)(b + 1) + size);
  new_b->sz = b->sz - size - sizeof(block);
  b->prev->next = new_b;
  new_b->prev = b->prev;
  new_b->next = b->next;
  b->next->prev = new_b;
  b->prev = NULL;
  b->next = NULL;
  b->sz = size;
  return 1;
}

int split_block_nolock(size_t size, block *b) {
  if (size > b->sz) {
    return 0; // if the left memory is not enough for second
  }
  if (size + sizeof(block) > b->sz) {

    th_blist->sz--;
    b->prev->next = b->next;
    b->next->prev = b->prev;
    b->next = NULL;
    b->prev = NULL;
    free_size -= sizeof(block) + b->sz;
    return 1; // if the left memory is not enough for second
  }
  // printf("reuse and divide\n");
  free_size -= sizeof(block) + size;
  block *new_b = (block *)((char *)(b + 1) + size);
  new_b->sz = b->sz - size - sizeof(block);
  b->prev->next = new_b;
  new_b->prev = b->prev;
  new_b->next = b->next;
  b->next->prev = new_b;
  b->prev = NULL;
  b->next = NULL;
  b->sz = size;
  return 1;
}
void merge_free_block(block *prev_block, block *cur_block) {
  if ((char *)(prev_block + 1) + prev_block->sz == (char *)(cur_block)) {
    prev_block->next = cur_block->next;
    cur_block->next->prev = prev_block;
    prev_block->sz = prev_block->sz + sizeof(block) + cur_block->sz;
    blist->sz--;
  }
  return;
}
void merge_free_block_nolock(block *prev_block, block *cur_block) {
  if ((char *)(prev_block + 1) + prev_block->sz == (char *)(cur_block)) {
    prev_block->next = cur_block->next;
    cur_block->next->prev = prev_block;
    prev_block->sz = prev_block->sz + sizeof(block) + cur_block->sz;
    th_blist->sz--;
  }
  return;
}

void add_to_list(block *b) {
   if ((char *)(blist->tail->prev) < (char *)(b) && blist->tail->prev!=blist->head) {
    blist->tail->prev->next = b;
    b->prev = blist->tail->prev;
    b->next = blist->tail;
    blist->tail->prev = b;
  } else {
    block *left = blist->head, *right = blist->head->next;
	 if(right==blist->tail){
		left->next = b;
		right->prev=b;
		b->next = right;
		b->prev = left;
	 }else{
		while (right != blist->tail) {
		if ((char *)(left) < (char *)(b) && (char *)(b) < (char *)(right)) {
			left->next = b;
			b->prev = left;
			b->next = right;
			right->prev = b;
			blist->sz++;
			return;
		}
		left = left->next;
		right = right->next;
		}
		if(right==th_blist->tail){
			left->next = b;
			right->prev = b;
			b->prev = left;
			b->next = right;
		}
		}
  }
  
  return;
}

void add_to_list_nolock(block *b) {
  // check the tail and its previous because the tail is not the last phy
  // address.
  if ((char *)(th_blist->tail->prev) < (char *)(b) && th_blist->tail->prev!=th_blist->head) {
    th_blist->tail->prev->next = b;
    b->prev = th_blist->tail->prev;
    b->next = th_blist->tail;
    th_blist->tail->prev = b;
  } else {
    block *left = th_blist->head, *right = th_blist->head->next;
	 if(right==th_blist->tail){
		left->next = b;
		right->prev=b;
		b->next = right;
		b->prev = left;
	 }else{
		while (right != th_blist->tail) {
		if ((char *)(left) < (char *)(b) && (char *)(b) < (char *)(right)) {
			left->next = b;
			b->prev = left;
			b->next = right;
			right->prev = b;
			th_blist->sz++;
			return;
		}
		left = left->next;
		right = right->next;
		}
		if(right==th_blist->tail){
			left->next = b;
			right->prev = b;
			b->prev = left;
			b->next = right;
		}
		}
  }
  
  return;
}

void ts_free_lock(void *ptr) {
  pthread_mutex_lock(&lk);
  if (ptr == NULL) {
    pthread_mutex_unlock(&lk);
    exit(EXIT_FAILURE);
  }
  block *b = (block *)((char *)(ptr) - sizeof(block));
  free_size += sizeof(block) + b->sz;
  add_to_list(b);
  merge_free_block(b, b->next);
  merge_free_block(b->prev, b);
  pthread_mutex_unlock(&lk);
  return;
}

void ts_free_nolock(void *ptr) {
 init_th_blist();
  if (ptr == NULL) {
    exit(EXIT_FAILURE);
  }
 
  block *b = (block *)((char *)(ptr) - sizeof(block));
  free_size += sizeof(block) + b->sz;

  add_to_list_nolock(b);

  merge_free_block_nolock(b, b->next);

  merge_free_block_nolock(b->prev, b);


  return;
}

void *ts_malloc_nolock(size_t size) {

  init_th_blist();

  block *closest_block = NULL;
  size_t closest_size = UINT_MAX;
  block *cur = th_blist->head->next;
  int alloc_success = 0;

  while (cur != th_blist->tail) {
    if (cur->sz == size) {
      split_block_nolock(size, cur);
      return cur + 1;
    }
    if (cur->sz > size) {
      if (closest_size > cur->sz - size) {
        closest_size = cur->sz - size;
        closest_block = cur;
      }
    }
    cur = cur->next;
  }

  // if there is no available block, we create a new block.size
  if (closest_block == NULL) {

    void *result = create_block_nolock(size);
    return result;
  } else {

    if (split_block_nolock(size, closest_block) == 1) {

      return closest_block+1;
    } else {

      void *result = create_block_nolock(size);
      return result;
    }
  }
  return NULL;
}

void *ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lk);
  init_blist();
  block *closest_block = NULL;
  size_t closest_size = UINT_MAX;
  block *cur = blist->head->next;
  int alloc_success = 0;
  while (cur != blist->tail) {
    if (cur->sz == size) {
      split_block(size, cur);
      pthread_mutex_unlock(&lk);
      return cur + 1;
    }
    if (cur->sz > size) {
      if (closest_size > cur->sz - size) {
        closest_size = cur->sz - size;
        closest_block = cur;
      }
    }
    cur = cur->next;
  }

  // if there is no available block, we create a new block.size
  if (closest_block == NULL) {
    void *result = create_block(size);
    pthread_mutex_unlock(&lk);
    return result;
  } else {
    if (split_block(size, closest_block) == 1) {
      pthread_mutex_unlock(&lk);
      return closest_block+1;
    } else {
      void *result = create_block(size);
      pthread_mutex_unlock(&lk);
      return result;
    }
  }
  pthread_mutex_unlock(&lk);
  return NULL;
}

// void add_to_list_nolock(block *b) {
//   // check the tail and its previous because the tail is not the last phy
//   // address.
//   if ((char *)(th_blist->tail->prev) < (char *)(b) && th_blist->tail->prev!=th_blist->head) {
// 	printf("goes branch 1\n");
//     th_blist->tail->prev->next = b;
//     b->prev = th_blist->tail->prev;
//     b->next = th_blist->tail;
//     th_blist->tail->prev = b;
//   } else {
// 		printf("goes branch 2\n");
//     block *left = th_blist->head, *right = th_blist->head->next;
// 	 printf("**************%lu\n", (size_t)right);
// 	 printf("***************%lu\n", (size_t)th_blist->tail);
// 	 if(right==th_blist->tail){
// 		// blist sz==0
// 		left->next = b;
// 		right->prev=b;
// 		b->next = right;
// 		b->prev = left;
// 	 }else{
// 		while (right != th_blist->tail) {
// 		if ((char *)(left) < (char *)(b) && (char *)(b) < (char *)(right)) {
// 			left->next = b;
// 			b->prev = left;
// 			b->next = right;
// 			right->prev = b;
// 			th_blist->sz++;
// 			return;
// 		}
// 		left = left->next;
// 		right = right->next;
// 		}
// 		if(right==th_blist->tail){
// 			left->next = b;
// 			right->prev = b;
// 			b->prev = left;
// 			b->next = right;
// 		}
// 		}
//   }
  
//   return;
// }
