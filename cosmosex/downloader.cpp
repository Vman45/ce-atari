#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

#include <signal.h>
#include <pthread.h>
#include <vector>    

#include <curl/curl.h>
#include "debug.h"
#include "downloader.h"
#include "utils.h"

// sudo apt-get install libcurl4-gnutls-dev
// gcc main.c -lcurl

extern volatile sig_atomic_t sigintReceived;

void splitFilenameFromPath(std::string &pathAndFile, std::string &path, std::string &file);
void mergeHostPaths(std::string &dest, std::string &tail);

pthread_mutex_t downloadThreadMutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<TDownloadRequest>   downloadQueue;
TDownloadRequest                downloadCurrent;

void Downloader::initBeforeThreads(void)
{
    curl_global_init(CURL_GLOBAL_ALL);                  // curl global init, must be called before any other threads start (not only curl threads)
    downloadCurrent.downPercent = 100;                  // mark that the current download is downloaded (shouldn't display it in status)
}

void Downloader::cleanupBeforeQuit(void)
{
    curl_global_cleanup();                              // curl global clean up, at the end...
}

void Downloader::add(TDownloadRequest &tdr)
{
	pthread_mutex_lock(&downloadThreadMutex);				// try to lock the mutex
    tdr.downPercent = 0;                                    // mark that the download didn't start
	downloadQueue.push_back(tdr);   						// add this to queue
	pthread_mutex_unlock(&downloadThreadMutex);			    // unlock the mutex
}

void Downloader::formatStatus(TDownloadRequest &tdr, std::string &line)
{
    char percString[16];

    std::string urlPath, fileName; 
    Utils::splitFilenameFromPath(tdr.srcUrl, urlPath, fileName);

    if(fileName.length() < 20) {                            // filename too short? extend to 20 chars with spaces
        fileName.resize(20, ' ');
    } else if(fileName.length() > 20) {                     // longer than 20 chars? make it shorter, add ... at the end
        fileName.resize(20);
        fileName.replace(17, 3, "...");
    }

    sprintf(percString, " % 3d %%", tdr.downPercent);

    line = fileName + percString;
}

void Downloader::status(std::string &status, int downloadTypeMask)
{
	pthread_mutex_lock(&downloadThreadMutex);				// try to lock the mutex

    std::string line;
    status.clear();

    // create status reports for things waiting to be downloaded
    int cnt = downloadQueue.size();
    for(int i=0; i<cnt; i++) {
        TDownloadRequest &tdr = downloadQueue[i];

        if((tdr.downloadType & downloadTypeMask) == 0) {    // if the mask doesn't match the download type, skip it
            continue;
        }

        formatStatus(tdr, line);
        status += line + "\n";
    }    

    // and for the currently downloaded thing
    if(downloadCurrent.downPercent < 100) {
        if((downloadCurrent.downloadType & downloadTypeMask) != 0) {    // if the mask matches the download type, add it
            formatStatus(downloadCurrent, line);
            status += line + "\n";
        }
    }

	pthread_mutex_unlock(&downloadThreadMutex);			    // unlock the mutex
}

int Downloader::count(int downloadTypeMask)
{
	pthread_mutex_lock(&downloadThreadMutex);				// try to lock the mutex

    int typeCnt = 0;

    // create status reports for things waiting to be downloaded
    int cnt = downloadQueue.size();
    for(int i=0; i<cnt; i++) {
        TDownloadRequest &tdr = downloadQueue[i];

        if((tdr.downloadType & downloadTypeMask) == 0) {    // if the mask doesn't match the download type, skip it
            continue;
        }

        typeCnt++;                                          // increment count
    }    

    // and for the currently downloaded thing
    if(downloadCurrent.downPercent < 100) {
        if((downloadCurrent.downloadType & downloadTypeMask) != 0) {    // if the mask matches the download type, add it
            typeCnt++;                                      // increment count
        }
    }

	pthread_mutex_unlock(&downloadThreadMutex);			    // unlock the mutex

    return typeCnt;
}

bool Downloader::verifyChecksum(char *filename, WORD checksum)
{
    if(checksum == 0) {                 // special case - when checksum is 0, don't check it buy say that it's OK
        return true;
    }

    FILE *f = fopen(filename, "rb");

    if(!f) {
        return false;
    }

    WORD cs = 0;
    WORD val, val2;

    while(!feof(f)) {                       // for whole file
        val = (BYTE) fgetc(f);              // get upper byte
        val = val << 8;

        if(!feof(f)) {                      // if not end of file
            val2 = (BYTE) fgetc(f);         // read lowe byte from file
        } else {
            val2 = 0;                       // if end of file, put a 0 there
        }

        val = val | val2;                   // create a word out of it

        cs += val;                          // add it to checksum
    }

    fclose(f);

    return (checksum == cs);                // return if the calculated cs is equal to the provided cs
}

static size_t my_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fwrite(ptr, size, nmemb, stream);
}
 
static size_t my_read_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fread(ptr, size, nmemb, stream);
}

static int my_progress_func(void *clientp, double downTotal, double downNow, double upTotal, double upNow)
{
    double percD;
    int percI;
    static int percIprev = 0;

    int abortTransfer = 0;                                      // by default - don't abort transfer
    if(sigintReceived != 0) {                                   // but if we did receive SIGINT, set abortTransfer
        abortTransfer = 1;
    }

    if(downTotal > 0.0) {
        percD = (downNow * 100.0) / downTotal;                  // calculate the percents
    } else {
        percD = 0.0;
    }

    percI = (int) percD;                                        // convert them to int

    if(percI == percIprev) {                                    // if percents didn't change, quit
        return abortTransfer;                                   // return 0 to continue transfer, or non-0 to abort transfer
    }
    percIprev = percI;

    if(clientp != NULL) {                                       // if got pointer to currently downloading request
    	pthread_mutex_lock(&downloadThreadMutex);				// try to lock the mutex
        TDownloadRequest *tdr = (TDownloadRequest *) clientp;   // convert pointer type
        tdr->downPercent = percI;                               // mark that the download didn't start
    	pthread_mutex_unlock(&downloadThreadMutex);			    // unlock the mutex
    }
    
    return abortTransfer;                                       // return 0 to continue transfer, or non-0 to abort transfer
}

void *downloadThreadCode(void *ptr)
{
    CURL *curl = NULL;
    CURLcode cres;
    int res;
    FILE *outfile;

	Debug::out("Download thread starting...");

	while(sigintReceived == 0) {
		pthread_mutex_lock(&downloadThreadMutex);		// lock the mutex

		if(downloadQueue.size() == 0) {					// nothing to do?
			pthread_mutex_unlock(&downloadThreadMutex);	// unlock the mutex
			sleep(1);									// wait 1 second and try again
			continue;
		}
		
		downloadCurrent = downloadQueue.front();	    // get the 'oldest' element from queue
		downloadQueue.erase(downloadQueue.begin());		// and remove it from queue
		pthread_mutex_unlock(&downloadThreadMutex);		// unlock the mutex

        curl = curl_easy_init();
        if(!curl) {
            Debug::out("CURL init failed, the file %s was not downloaded...", (char *) downloadCurrent.srcUrl.c_str());
            continue;
        }
    
        std::string urlPath, fileName; 
        Utils::splitFilenameFromPath(downloadCurrent.srcUrl, urlPath, fileName);
    
        std::string tmpFile, finalFile;

        finalFile = downloadCurrent.dstDir;
        Utils::mergeHostPaths(finalFile, fileName);         // create final local filename with path

        tmpFile = finalFile + "_dwnldng";                   // create temp local filename with path
        outfile = fopen((char *) tmpFile.c_str(), "wb");    // try to open the tmp file

        if(!outfile) {
            Debug::out("Downloader - failed to create local file: %s", (char *) fileName.c_str());
            continue;
        }

        Debug::out("Downloader - download remote file: %s to local file: %s", (char *) downloadCurrent.srcUrl.c_str(), (char *) fileName.c_str());
     
        // now configure curl
        curl_easy_setopt(curl, CURLOPT_URL,                 (char *) downloadCurrent.srcUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,           outfile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,       my_write_func);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION,        my_read_func);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS,          0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,    my_progress_func);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,        &downloadCurrent);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR,         true);
 
        cres = curl_easy_perform(curl);                      // start the transfer
 
        fclose(outfile);

        if(cres == CURLE_OK) {                               // if download went OK
            bool b = Downloader::verifyChecksum((char *) tmpFile.c_str(), downloadCurrent.checksum);

            if(b) {         // checksum is OK? 
                res = rename(tmpFile.c_str(), finalFile.c_str());

                if(res != 0) {
                    Debug::out("Downloader - failed to rename %s to %s after download", (char *) tmpFile.c_str(), (char *) finalFile.c_str());
                } else {
                    Debug::out("Downloader - file %s was downloaded with success.", (char *) downloadCurrent.srcUrl.c_str());
                }
            } else {
                res = remove(tmpFile.c_str());

                if(res == 0) {
                    Debug::out("Downloader - file %s was downloaded, but verifyChecksum() failed, so file %s was deleted.", (char *) tmpFile.c_str(), (char *) tmpFile.c_str());
                } else {
                    Debug::out("Downloader - file %s was downloaded, but verifyChecksum() failed, and then failed to delete that file.", (char *) tmpFile.c_str());
                }
            }

            downloadCurrent.downPercent = 100;
        } else {                                            // if download failed
            Debug::out("Downloader - download of %s didn't finish with success, deleting incomplete file.", (char *) downloadCurrent.srcUrl.c_str());

            res = remove(tmpFile.c_str());

            if(res != 0) {
                Debug::out("Downloader - failed to delete file %s", (char *) tmpFile.c_str());
            }
        }

        curl_easy_cleanup(curl);
        curl = NULL;
    }

	Debug::out("Download thread finished.");
    return 0;
}


