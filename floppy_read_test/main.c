#include <stdio.h>
#include <osbind.h>

#include "stdlib.h"

void showMenu(void);

void readSector(int sector, int track, int side);
void writeSector(int sector, int track, int side);

void showBiosError(int errorNo);
void showDecimal(int num);

BYTE writeBfr[512];

int main(void)
{
	int track, sector, side;
	int no, i;
	char req;
    int readNotWrite;

    for(i=0; i<512; i++) {
        writeBfr[i] = (BYTE) i;
    }
	
	track	= 0;
	side	= 0;
	sector	= 1;
	
    showMenu();
    
	while(1) {
		req = Cnecin();
        readNotWrite = 1;                           // 1 means READ, 0 means WRITE
		
		/* should quit? */
		if(req == 'q' || req == 'Q' || req=='e' || req=='E') {	
			return 0;
		}
        
        if(req == 'M' || req == 'm') {
            showMenu();
            continue;
        }
		
		if(req == 't' || req == 'T') {				/* set track # */
			no = ((BYTE) Cnecin()) - 48;
			if(no >= 0 && no < 85) {
				track = no;
			}
		} else if(req == 'i' || req == 'I') {		/* set side # */
			no = ((BYTE) Cnecin()) - 48;
			if(no >= 0 && no <= 1) {
				side = no;
			}
		} else if(req == 's' || req == 'S') {		/* set sector # */
			no = ((BYTE) Cnecin()) - 48;
			if(no >= 1 && no <= 12) {
				sector = no;
			}
		} else if(req == 'j' || req == 'J') {		/* jump up by 20 track */
			if((track + 20) < 80) { 
				track += 20;
			} else {
				track = 0;
			}			
			printf("Jumped up to track %d\n", track);
		} else if(req == 'k' || req == 'K') {		/* jump down by 20 track */
			if((track - 20) >= 0) { 
				track -= 20;
			} else {
				track = 79;
			}			
			printf("Jumped down to track %d\n", track);
		} else if(req == 'r' || req == 'R') {		/* jump down by 20 track */
			track	= 0;
			side	= 0;
			sector	= 1;
			printf("Reset to track 0, side 0, sector 1\n");
		} else if(req == 'w' || req == 'W') {       // write sector
            readNotWrite = 0;
        }        
        
        if(readNotWrite == 1) {                     // 1 means READ, 0 means WRITE
            readSector(sector, track, side);
        } else {
            writeSector(sector, track, side);
        }
	}
}

void showMenu(void)
{
	printf("%c%cManual floppy sector requests tool.\n", 27, 69);
    printf("Commands: 'q' to quit, set sector: 's1',\n");
    printf("set side: 'i0', set track: 't5'\n");
    printf("'j' - jump up, 'k' - jump down, 'r' - go to track 0\n");
    printf("'w' - write currently set sector, 'm' - this menu\n\n");
}

void readSector(int sector, int track, int side)
{
	BYTE bfr[512];
	int res;

	res = Floprd(bfr, 0, 0, sector, track, side, 1);
				
    printf("READ  Tr %d, Si %d, Se %d -- ", track, side, sector);

	if(res != 0) {
		printf("FAIL -- res = %d\n", res);
		showBiosError(res);
	} else {
		if(bfr[0] == track && bfr[1] == side && bfr[2] == sector) {
			printf("GOOD, TrSiSe GOOD\n");
		} else {
			printf("GOOD, TrSiSe BAD\n");
		}
	}
}

void writeSector(int sector, int track, int side)
{
	int res;

    // customize write data
    writeBfr[0] = track;
    writeBfr[1] = side;
    writeBfr[2] = sector;
    
    // issue write command
	res = Flopwr(writeBfr, 0, 0, sector, track, side, 1);
                
    printf("WRITE Tr %d, Si %d, Se %d -- ", track, side, sector);

	if(res != 0) {
		printf("FAIL -- res = %d\n", res);
		showBiosError(res);
	} else {
        printf("GOOD\n");
	}
}

void showBiosError(int errorNo)
{
	switch(errorNo) {
		case 0:   printf("No error\n"); break;
		case -1:  printf("BIOS Error: Generic error\n"); break;
		case -2:  printf("BIOS Error: Drive not ready\n"); break;
		case -3:  printf("BIOS Error: Unknown command\n"); break;
		case -4:  printf("BIOS Error: CRC error\n"); break;
		case -5:  printf("BIOS Error: Bad request\n"); break;
		case -6:  printf("BIOS Error: Seek error\n"); break;
		case -7:  printf("BIOS Error: Unknown media\n"); break;
		case -8:  printf("BIOS Error: Sector not found\n"); break;
		case -9:  printf("BIOS Error: Out of paper\n"); break;
		case -10: printf("BIOS Error: Write fault\n"); break;
		case -11: printf("BIOS Error: Read fault\n"); break;
		case -12: printf("BIOS Error: Device is write protected\n"); break;
		case -14: printf("BIOS Error: Media change detected\n"); break;
		case -15: printf("BIOS Error: Unknown device\n"); break;
		case -16: printf("BIOS Error: Bad sectors on format\n"); break;
		case -17: printf("BIOS Error: Insert other disk (request)\n"); break;
		default:  printf("BIOS Error: something other...\n"); break;
	}
}

