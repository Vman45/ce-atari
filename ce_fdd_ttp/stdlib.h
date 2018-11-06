#ifndef _STDLIB_H_
#define _STDLIB_H_

#include "global.h"

void *	memcpy ( void * destination, const void * source, int num );
void *	memset ( void * ptr, int value, int num );
int		strlen ( const char * str );
char *	strncpy ( char * destination, const char * source, int num );
char *  strcpy( char * destination, const char * source);
char *  strcat( char * destination, const char * source);
int		strncmp ( const char * str1, const char * str2, int num );
void	sleep(int seconds);
void    msleep  ( int ms );
DWORD   getTicks(void);
BYTE    atariKeysToSingleByte(BYTE vkey, BYTE key);
BYTE    getKeyIfPossible(void);

#endif
