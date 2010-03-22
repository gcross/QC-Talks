#include "bloom.h"
#include <stdio.h>

static int check_set_bit(bloom_filter *b, int pos) {
  bloom_filter bit = (0x1UL << pos);
  if((*b & bit)) {
    return(0);
  } else {
    *b |= bit;
    return(1);
  }
}

int bloom_check_insert(bloom_filter *b, int value, int numbits) {
  int hash = value ^ (value >> 5) ^ (value << 20) ^ (value << 12);
  int i;
  int new_bit = 0;
  // printf("0x%x -> 0x%x\n", value, hash);

  for(i=0; i<numbits; i++) {
    new_bit |= check_set_bit(b, hash & 0x3F);
    hash >>= 6;
  }
  return(new_bit);
}

