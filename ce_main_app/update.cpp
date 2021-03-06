#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>

#include "global.h"
#include "debug.h"
#include "settings.h"
#include "downloader.h"
#include "update.h"
#include "utils.h"
#include "mounter.h"
#include "dir2fdd/cdirectory.h"
#include "floppy/imagelist.h"

Versions Update::versions;

extern THwConfig hwConfig;

volatile BYTE packageDownloadStatus     = DWNSTATUS_WAITING;
volatile BYTE updateListDownloadStatus  = DWNSTATUS_WAITING;

void Update::initialize(void)
{
    char appVersion[16];
    Version::getAppVersion(appVersion);

    Update::versions.current.app.fromString(                (char *) appVersion);
    Update::versions.current.xilinx.fromFirstLineOfFile(    (char *) XILINX_VERSION_FILE, false);   // xilinx version file without dashes
    Update::versions.current.imageList.fromFirstLineOfFile( (char *) IMAGELIST_LOCAL);
    Update::versions.updateListWasProcessed = false;
    Update::versions.gotUpdate              = false;
}

bool Update::createUpdateScript(bool withLinuxRestart, bool withForceXilinx)
{
    bool res;

    if(withForceXilinx) {       // should force xilinx flash?
        res = writeSimpleTextFile(UPDATE_FLASH_XILINX, NULL);     // create this file to force xilinx flash

        if(!res) {              // if failed to create first file, just quit already
            return res;
        }
    }

    // write the main update script command
    // don't forget to pass 'nosystemctl dontkillcesuper' do ce_update.sh here, because we need those to tell ce_stop.sh (which is called in ce_update.sh)
    // that it shouldn't stop cesuper.sh, which we need to keep running to finish the update and restart ce_main_app back
    res = writeSimpleTextFile(UPDATE_SCRIPT, "#!/bin/sh\n/ce/ce_update.sh nosystemctl dontkillcesuper\n");

    if(!res) {                  // if failed to create first file, just quit already
        return res;
    }

    if(withLinuxRestart) {      // if should also restart linux, add reboot
        res = writeSimpleTextFile(UPDATE_REBOOT_FILE, NULL);
    }

    return res;
}

bool Update::createUpdateXilinxScript(void)
{
    bool res = createUpdateScript(false, true);     // don't reboot, force xilinx flash
    return res;
}

bool Update::createFlashFirstFwScript(bool withLinuxRestart)
{
    bool res = createUpdateScript(withLinuxRestart, false);     // reboot if requested, don't force xilinx flash
    return res;
}

bool Update::checkForUpdateListOnUsb(std::string &updateFilePath)
{
    DIR *dir = opendir((char *) "/mnt/");                           // try to open the dir

    updateFilePath.clear();

    if(dir == NULL) {                                               // not found?
        Debug::out(LOG_DEBUG, "Update::checkForUpdateListOnUsb -- /mnt/ dir not found, fail");
        return false;
    }

    char path[512];
    memset(path, 0, 512);
    bool found = false;

    while(1) {                                                      // while there are more files, store them
        struct dirent *de = readdir(dir);                           // read the next directory entry

        if(de == NULL) {                                            // no more entries?
            break;
        }

        if(de->d_type != DT_DIR && de->d_type != DT_REG) {          // not a file, not a directory?
            continue;
        }

        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        // construct path
        strcpy(path, "/mnt/");
        strcat(path, de->d_name);
        strcat(path, "/");
        strcat(path, getUsbArchiveName());

        found = Utils::fileExists(path);

        if(found) {                                             // found? good
            break;
        }
    }

    closedir(dir);

    if(found) {
        Debug::out(LOG_DEBUG, "Update::checkForUpdateListOnUsb -- update found: %s", path);
        updateFilePath = path;
        writeSimpleTextFile(UPDATE_USBFILE, path);              // also write this path to predefined file for ce_update.sh script
    } else {
        Debug::out(LOG_DEBUG, "Update::checkForUpdateListOnUsb -- update not found on usb");
    }

    return found;
}

const char *Update::getUsbArchiveName(void)
{
    #ifdef DISTRO_YOCTO
        return "yocto.zip";
    #endif

    #ifdef DISTRO_JESSIE
        return "jessie.zip";
    #endif

    #ifdef DISTRO_STRETCH
        return "stretch.zip";
    #endif

    return "unknown.zip";
}

const char *Update::getPropperXilinxTag(void)
{
    if(hwConfig.version == 1) {                 // v.1 ? it's xilinx
        return "xilinx";
    } else {                                    // v.2 ?
        if(hwConfig.hddIface == HDD_IF_ACSI) {  // v.2 and ACSI? it's xlnx2a
            return "xlnx2a";
        } else {                                // v.2 and SCSI? it's xlnx2s
            return "xlnx2s";
        }
    }

    return "??wtf??";
}

void Update::createFloppyTestImage(void)
{
    // open the file and write to it
    FILE *f = fopen(FDD_TEST_IMAGE_PATH_AND_FILENAME, "wb");

    if(!f) {
        Debug::out(LOG_ERROR, "Failed to create floppy test image!");
        printf("Failed to create floppy test image!\n");
        return;
    }

    // first fill the write buffer with simple counter
    BYTE writeBfr[512];
    int i;
    for(i=0; i<512; i++) {
        writeBfr[i] = (BYTE) i;
    }

    // write one sector after another...
    int sector, track, side;
    for(track=0; track<80; track++) {
        for(side=0; side<2; side++) {
            for(sector=1; sector<10; sector++) {
                // customize write data
                writeBfr[0] = track;
                writeBfr[1] = side;
                writeBfr[2] = sector;

                fwrite(writeBfr, 1, 512, f);
            }
        }
    }

    // close file and we're done
    fclose(f);
}

bool Update::writeSimpleTextFile(const char *path, const char *content)
{
    FILE *f = fopen(path, "wt");

    if(!f) {                // if couldn't open file, quit
        Debug::out(LOG_ERROR, "Update::writeSimpleTextFile failed to create file: %s", path);
        return false;
    }

    if(content) {           // if content specified, write it (otherwise empty file)
        fputs(content, f);
    }

    fclose(f);
    return true;
}

void Update::removeSimpleTextFile(const char *path)
{
    unlink(path);
}

void Update::createNewScripts(void)
{
    // avoid running more than once by a simple bool
    static bool wasRunOnce = false;         // make sure this runs only once - not needed to run it more times

    if(wasRunOnce) {                        // if it was already runned, quit
        Debug::out(LOG_DEBUG, "Update::createNewScripts() - won't try to update scripts, already tried that once during this app run");
        return;
    }

    wasRunOnce = true;                      // mark that we've runned this once

    // check if the git hidden directory exists, and if it does, then don't copy the scripts - it will be all handled by git
    struct stat sb;

    if (stat("/ce/.git", &sb) == 0 && S_ISDIR(sb.st_mode)) {    // path exists and it's a dir, quit
        Debug::out(LOG_DEBUG, "Update::createNewScripts() - /ce/ dir is already under git, won't try to update scripts");
        return;
    }

    // it seems that we should update the scripts, so update them
    printf("Will try to update scripts\n");
    Debug::out(LOG_DEBUG, "Update::createNewScripts() - will try to update scripts");

    // run the script
    system("chmod 755 /ce/app/shellscripts/copynewscripts.sh");             // make the copying script executable
    system("/ce/app/shellscripts/copynewscripts.sh ");                      // execute the copying script
}
