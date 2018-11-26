#include <mint/sysbind.h>
#include <mint/osbind.h>

#include "acsi.h"
#include "stdlib.h"

void *memcpy ( void * destination, const void * source, int num )
{
	BYTE *dst = (BYTE *) destination;
	BYTE *src = (BYTE *) source;
	int i;
	
	for(i=0; i<num; i++) {				// copy all from src to dst
		dst[i] = src[i];
	}
	
	return destination;
}

void *memset ( void * ptr, int value, int num )
{
	BYTE *p = (BYTE *) ptr;
	int i;
	
	for(i=0; i<num; i++) {				// set all in ptr to value
		p[i] = value;
	}
	
	return ptr;
}

int strlen ( const char * str )
{
	int i;

	for(i=0; i<2048; i++) {				// find first zero and return it's position
		if(str[i] == 0) {
			return i;
		}
	}
	
	return 0;
}

char *strncpy ( char * destination, const char * source, int num )
{
	int i;
	
	for(i=0; i<num; i++) {				// copy max. num chars, but even less when termination zero is found
		destination[i] = source[i];
		
		if(source[i] == 0) {			// terminating zero found?
			break;
		}
	}
	
	return destination;
}

char *strcpy( char * destination, const char * source)
{
	int i;
	
	for(i=0; i<1024; i++) {				
		destination[i] = source[i];
		
		if(source[i] == 0) {			// terminating zero found?
			break;
		}
	}
	
	return destination;
}

char *strcat( char * destination, const char * source)
{
    int len = strlen(destination);
    
    strcpy(destination + len, source);
    
    return destination;
}

int strncmp ( const char * str1, const char * str2, int num )
{
	int i;

	for(i=0; i<num; i++) {			
		if(str1[i] == str2[i]) {			// chars matching? continue
			continue;
		}

		if(str1[i] == 0 && str2[i] != 0) {	// 1st string terminated, 2nd string continues? 
			return -1; 
		}
		
		if(str1[i] != 0 && str2[i] == 0) {	// 1st string continues, 2nd string terminated? 
			return 1; 
		}
		
		if(str1[i] > str2[i]) {
			return 1;
		} else {
			return -1;
		}
	}
	
	return 0;							// if came here, all chars matching
}

int sleepSeconds;
static void sleepInSupervisor(void);

void sleep(int seconds)
{
	sleepSeconds = seconds;
	Supexec(sleepInSupervisor);
}

static void sleepInSupervisor(void)
{
	DWORD now, until;
	DWORD tics = sleepSeconds * 200;

	now = getTicks();						// get current ticks
	until = now + tics;   					// calc value timer must get to

	while(1) {
		now = getTicks();					// get current ticks
		
		if(now >= until) {
			break;
		}
	}
}

DWORD getTicks(void)
{
    DWORD now;
    now = *HZ_200;
    return now;
}

DWORD getTicksAsUser(void)
{
    DWORD res = Supexec(getTicks);
    return res;
}

void showHexByte(BYTE val)
{
    int hi, lo;
    char tmp[3];
    char table[16] = {"0123456789ABCDEF"};

    hi = (val >> 4) & 0x0f;;
    lo = (val     ) & 0x0f;

    tmp[0] = table[hi];
    tmp[1] = table[lo];
    tmp[2] = 0;

    (void) Cconws(tmp);
}

void showHexWord(WORD val)
{
    showHexByte(val >> 8);
    showHexByte(val & 0xff);
}

void showHexDword(DWORD val)
{
    showHexByte(val >> 24);
    showHexByte(val >> 16);
    showHexByte(val >>  8);
    showHexByte(val      );
}

