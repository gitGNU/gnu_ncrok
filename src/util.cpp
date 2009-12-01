#include <sys/types.h>
#include "util.h"

size_t strcopy( char *dst, const char *src, size_t length ){
	size_t count = length;
	while(*src != 0 && --count > 0)
		*(dst++) = *(src++);
	*dst = 0;
	return length - count;
}

int max( int a, int b ){
	return (a > b)? a : b;
}

int min( int a, int b ){
	return (a < b)? a : b;
}
