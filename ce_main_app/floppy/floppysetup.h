#ifndef _FLOPPYSETUP_H_
#define _FLOPPYSETUP_H_

#include <stdio.h>

#include "../settingsreloadproxy.h"
#include "../datatypes.h"
#include "../translated/translateddisk.h"
#include "../settingsreloadproxy.h"

#define IMG_DN_STATUS_IDLE          0
#define IMG_DN_STATUS_DOWNLOADING   1
#define IMG_DN_STATUS_DOWNLOADED    2

class DataTrans;

class FloppySetup
{
public:
    FloppySetup();
    ~FloppySetup();

    void processCommand(BYTE *command);

    void setDataTrans(DataTrans *dt);
    void setSettingsReloadProxy(SettingsReloadProxy *rp);

    static bool createNewImage(std::string pathAndFile);

private:
    DataTrans           *dataTrans;
    SettingsReloadProxy *reloadProxy;

    FILE    *up;
    BYTE    *cmd;

    BYTE    bfr64k[64 * 1024 + 4];

    int         imgDnStatus;
    std::string inetDnFilePath;
    std::string inetDnFilename;

    struct {
        int         slotIndex;
        std::string atariSourcePath;
        std::string hostSourcePath;
        std::string hostDestinationPath;
        std::string file;
        FILE *fh;
    } currentUpload;

    struct {
        std::string hostDestPath;
        std::string imageFile;
        FILE *fh;
    } currentDownload;

    void uploadStart(void);
    void uploadBlock(void);
    void uploadEnd(bool isOnDeviceCopy);

    void newImage(void);
    void getNewImageName(char *nameBfr);

    void downloadStart(void);
    void downloadGetBlock(void);
    void downloadDone(void);
    void downloadOnDevice(void);

    void searchInit(void);
    void searchString(void);
    void searchResult(void);
    void searchRefreshList(void);

    void searchMark(void);              // OBSOLETE
    void searchDownload(void);          // OBSOLETE

    void searchDownload2Storage(void);  // CURRENT WAY OF DOWNLOADING
    void searchInsertToSlot(void);
    void searchDeleteFromStorage(void);

    void getCurrentSlot(void);
    void setCurrentSlot(void);
    void getImageEncodingRunning(void);

    void logCmdName(BYTE cmdCode);
};

#endif


