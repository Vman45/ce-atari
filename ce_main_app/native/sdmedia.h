#ifndef _SDMEDIA_H
#define _SDMEDIA_H

#include <stdio.h>
#include "../datatypes.h"
#include "imedia.h"

class SdMedia: public IMedia
{
public:
    SdMedia();
    virtual ~SdMedia();

	void setCurrentCapacity(DWORD sectors);
	
    virtual bool iopen(const char *path, bool createIfNotExists);
    virtual void iclose(void);

    virtual bool isInit(void);
    virtual bool mediaChanged(void);
    virtual void setMediaChanged(bool changed);
    virtual void getCapacity(int64_t &bytes, int64_t &sectors);

    virtual bool readSectors(int64_t sectorNo, DWORD count, BYTE *bfr);
    virtual bool writeSectors(int64_t sectorNo, DWORD count, BYTE *bfr);

private:
	int64_t	capacityInSectors;
	bool	mediaChangedFlag;
	BYTE	senseKey;
	
};

#endif // _SDMEDIA_H
