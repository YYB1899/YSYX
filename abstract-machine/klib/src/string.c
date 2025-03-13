#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  //panic("Not implemented");
  size_t n = 0;
  while(*s != '\0'){
  	n ++;
  	s ++;
  }
  return n;
}

char *strcpy(char *dst, const char *src) {
  //panic("Not implemented");
  char* ret = dst;
  while(*src != '\0'){
	*dst = *src;
	src ++;
	dst ++;
   }
  *dst = '\0';
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  //panic("Not implemented");
  char* ret = dst;
  while(n > 0 && *src != '\0'){
  	*dst = *src;
  	src ++;
  	dst ++;
  	n --;
  }
  while(n > 0){
  	*dst = '\0';
  	dst ++;
  	n --;
  }
  return ret;
}

char *strcat(char *dst, const char *src) {
  //panic("Not implemented");
  char* ret = dst;
  while(*dst != '\0'){
  	dst ++;
  }
  while(*src != '\0'){
  	*dst = *src;
  	dst ++;
  	src ++;
  }
  *dst = '\0';
  return ret;
}

int strcmp(const char *s1, const char *s2) {
  //panic("Not implemented");
  while(*s1 != '\0' && *s2 != '\0'){
  	if(*s1 != *s2){
		return (*s1 - *s2);    
  	}
  	s1 ++;
  	s2 ++;
  }
  return (*s1 - *s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
  //panic("Not implemented");
  while(n > 0 && *s1 != '\0' && *s2 != '\0'){
	if(*s1 != *s2){
		return (*s1 - *s2);
	}
	s1 ++;
	s2 ++;
	n --;
  }
  if(n == 0){
  	return 0;
  }
  //return (*s1 - *s2); 
  return 0;
}

void *memset(void *s, int c, size_t n) {
  //panic("Not implemented");
  unsigned char *i = (unsigned char*)s;
  while(n > 0){
  	*i = (unsigned char)c;
  	i ++;
  	n --;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  //panic("Not implemented");
  unsigned char *d = (unsigned char*)dst;
  const unsigned char *s = (const unsigned char*)src;
  if(s < d && d < s + n){
  	for(size_t i = n;i > 0;i --){
		d[i - 1] = s[i - 1];
	}
  }else{
  	for(size_t i = 0; i < n; i ++){
  		d[i] = s[i];
  	}
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  //panic("Not implemented");
  unsigned char *o = (unsigned char*)out;
  const unsigned char *i = (const unsigned char*)in;
  while(n > 0){
  	*o = *i;
  	o ++;
  	i ++;
  	n --;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  //panic("Not implemented");
  unsigned char *a = (unsigned char*)s1;
  const unsigned char *b = (const unsigned char*)s2;
  while(n > 0){
  	if(*a != *b){
  		return (*a - *b);
  	}
  	a ++;
  	b ++;
	n --;
  }
  return 0;
}

#endif
