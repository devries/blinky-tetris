#include "hashtable.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char *test = "Hello";
  hashtable *hashtbl;
  int i;
  char letter;
  int *ip;
  hashtable_iterator *iterator;
  char *key;

  hashtbl = hashtable_create(8,NULL);
  if(!hashtbl) {
    fprintf(stderr, "Table allocation error\n");
  }

  letter = 'a';
  i=1;
  if(hashtable_insert(hashtbl,&letter,sizeof(letter),&i,sizeof(i))<0) {
    fprintf(stderr, "Did not add to hash correctly\n");
  }
  letter = 'b';
  i=2;
  if(hashtable_insert(hashtbl,&letter,sizeof(letter),&i,sizeof(i))<0) {
    fprintf(stderr, "Did not add to hash correctly\n");
  }
  letter = 'c';
  i=3;
  if(hashtable_insert(hashtbl,&letter,sizeof(letter),&i,sizeof(i))<0) {
    fprintf(stderr, "Did not add to hash correctly\n");
  }
  letter = 'a';
  i=4;
  if(hashtable_insert(hashtbl,&letter,sizeof(letter),&i,sizeof(i))<0) {
    fprintf(stderr, "Did not change hash correctly\n");
  }

  ip = (int*)hashtable_get(hashtbl,&letter,sizeof(letter));
  printf("Integer for 'a': %d (should be 4)\n", *ip);

  letter = 'c';
  if(hashtable_remove(hashtbl,&letter,sizeof(letter))<0) {
    fprintf(stderr, "Unable to remove letter 'c'\n");
  }
  
  ip = (int*)hashtable_get(hashtbl,&letter,sizeof(letter));
  if(ip==NULL) {
    printf("After removal, returning NULL as expected.\n");
  }
  else {
    printf("Not null after removal!!!!\n");
  }

  letter = 'b';
  ip = (int*)hashtable_get(hashtbl,&letter,sizeof(letter));
  printf("For 'b' we get %d (should be 2)\n", *ip);

  printf("Inserting from a pointer to literal (C99)\n");
  i=5;
  if(hashtable_insert(hashtbl,&(char){'e'},sizeof(char),&i,sizeof(i))<0) {
    fprintf(stderr,"Literal insertion failure.\n");
  }
  ip = (int*)hashtable_get(hashtbl,&(char){'e'},sizeof(char));
  printf("Integer for literal 'e': %d (should be 5)\n",*ip);

  printf("Iterate through hashtable\n");
  iterator = hashtable_iterator_create(hashtbl);
  hashtable_iterator_next(iterator);
  while((key=(char*)hashtable_iterator_get_key(iterator))!=NULL) {
    ip = (int*)hashtable_iterator_get_data(iterator);
    printf("'%c' = %d\n",*key,*ip);
    hashtable_iterator_next(iterator);
  }
  hashtable_iterator_free(iterator);

  printf("Pathological Case.\n");

  for(i=0;i<100000;i++) {
    if(hashtable_insert(hashtbl,&i,sizeof(i),&i,sizeof(i))<0) {
      fprintf(stderr, "Insertion error at %d\n",i);
    }
  }

  for(i=0;i<100000;i+=10000) {
    ip = (int*)hashtable_get(hashtbl,&i,sizeof(i));
    if(ip==NULL) {
      fprintf(stderr, "Retrieval error at %d\n",i);
    }
    else {
      printf("%d=%d\n",i,*ip);
    }
  }

  hashtable_free(hashtbl);
  
  printf("size of uint8_t = %lu\n", sizeof(uint8_t));
  printf("length of \"%s\" = %lu\n",test,strlen(test));
  printf("Hash without null: %"PRIu64"\n",fnv1a64(test,strlen(test)));
  printf("Hash with null: %"PRIu64"\n",fnv1a64(test,strlen(test)+1));
  exit(0);
}
