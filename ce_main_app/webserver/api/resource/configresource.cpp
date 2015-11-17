#include "configresource.h"
#include "webserver/webserver.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/types.h>

#include "../../../lib/cjson-code-58/cJSON.h"
#include "../../../global.h"
#include "../../../datatypes.h"
#include "../../../debug.h"
#include "../../../utils.h"
#include "../../../config/configstream.h"
#include "../../../ce_conf_on_rpi.h"

extern int translateVT52toVT100(BYTE *bfr, BYTE *tmp, int cnt);
#define INBFR_SIZE  (100 * 1024)

static int webFd1;
static int webFd2;

ConfigResource::ConfigResource()  
{
    in_bfr  = new BYTE[INBFR_SIZE];
    tmp_bfr = new BYTE[INBFR_SIZE];
    
    webFd1  = -1;
    webFd2  = -1;
}

ConfigResource::~ConfigResource() 
{
    if(webFd1 > 0) {
        close(webFd1);
    }
    
    if(webFd2 > 0) {
        close(webFd2);
    }        

    delete[] in_bfr;
    delete[] tmp_bfr;
}

void ConfigResource::openFIFOsIfNeeded(void)
{
    if(webFd1 <= 0) {
        webFd1 = open(FIFO_WEB_PATH1, O_RDWR);      // will be used for writing only
    }
    
    if(webFd2 <= 0) {
        webFd2 = open(FIFO_WEB_PATH2, O_RDWR);      // will be used for reading only
    }
}

bool ConfigResource::dispatch(mg_connection *conn, mg_request_info *req_info, std::string sResourceInfo /*=""*/ ) 
{
    Debug::out(LOG_DEBUG, "ConfigResource::dispatch");

    if( strstr(req_info->request_method,"GET")==0 && strstr(req_info->request_method,"POST")==0 ){
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed %s\r\n",req_info->request_method);
        return true;
    }

    //set loglevel
    if( strstr(req_info->request_method,"POST")>0 && sResourceInfo=="loglevel" ){
        Debug::out(LOG_DEBUG, "/config/loglevel POST");

        char post_data[1024], loglevel[sizeof(post_data)];
        int post_data_len = mg_read(conn, post_data, sizeof(post_data));

        // Parse form data. input1 and input2 are guaranteed to be NUL-terminated
        mg_get_var(post_data, post_data_len, "loglevel", loglevel, sizeof(loglevel));
        std::string sLogLevel=std::string(loglevel);
        if( sLogLevel!="ll1" && sLogLevel!="ll2" && sLogLevel!="ll3" && sLogLevel!="" )
        {
            mg_printf(conn, "HTTP/1.1 400 unknown loglevel\r\n");
            return true;
        }

        std::ofstream fileLogLevel("/tmp/ce_startupmode");
        fileLogLevel << sLogLevel;

        mg_printf(conn, "HTTP/1.1 200 OK\r\n\r\n");
        return true;    
    }
    //get configterminal screen
    if( strstr(req_info->request_method,"GET")>0 && sResourceInfo=="terminal" ){
        Debug::out(LOG_DEBUG, "/config/terminal GET");
        int iReadLen=sendTerminalCommand(CFG_CMD_SET_RESOLUTION, ST_RESOLUTION_HIGH);
        iReadLen=sendTerminalCommand(CFG_CMD_REFRESH, 0);
        if( iReadLen>=0 ){
            mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n");
            mg_printf(conn, "Cache: no-cache\r\n");
            mg_printf(conn, "Content-Length: %d\r\n\r\n",iReadLen);        // Always set Content-Length
            mg_write(conn, in_bfr,iReadLen );
            return true;
        } else{
            Debug::out(LOG_ERROR, "could not send config command");
            mg_printf(conn, "HTTP/1.1 500 Server Error - could not send config command\r\n\r\n");
            return true;
        }                
    }

    //return config info
    if( strstr(req_info->request_method,"GET")>0 && sResourceInfo=="" ){
        Debug::out(LOG_DEBUG, "/config GET");
        //const char *qs = req_info->query_string;
        mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
        mg_printf(conn, "Cache: no-cache\r\n");
        std::ostringstream stringStream;
        stringStream << "{\"config\":[";
        stringStream << "]}"; 
        std::string sJson=stringStream.str();
        mg_printf(conn, "Content-Length: %d\r\n\r\n",sJson.length());        // Always set Content-Length
        mg_printf(conn, sJson.c_str());        // Always set Content-Length
        return true;
    }

    //send configterminal command
    if( strstr(req_info->request_method,"POST")>0 && sResourceInfo=="terminal" ){
        Debug::out(LOG_DEBUG, "/config/terminal POST");

        int len=0;
        char jsonKey[1024]; 

        //raw read, not automaticly 0-terminated
        len = mg_read(conn, jsonKey, sizeof(jsonKey)-1);
        jsonKey[len]=0;  
        
        if( len>0 ){
            Debug::out(LOG_DEBUG, "json:%s",jsonKey);
            cJSON *root = cJSON_Parse(jsonKey);
            if( root!=NULL ){
                cJSON *pxCJSONObject=cJSON_GetObjectItem(root,"key");
                if( pxCJSONObject==NULL ){
                    Debug::out(LOG_ERROR, "key code missing");                      
                    mg_printf(conn, "HTTP/1.1 400 Bad Request - key code missing\r\n\r\n");
                    return true;
                }
                int iCode = pxCJSONObject->valueint; 
                if( iCode==0 ){
                    Debug::out(LOG_ERROR, "invalid key code: %d",iCode);
                    mg_printf(conn, "HTTP/1.1 400 Bad Request - invalid key code\r\n\r\n");
                    return true;
                }
                int iReadLen=sendTerminalCommand(CFG_CMD_KEYDOWN, iCode);
                if( iReadLen>=0 ){
                    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n");
                    mg_printf(conn, "Cache: no-cache\r\n");
                    mg_printf(conn, "Content-Length: %d\r\n\r\n",iReadLen);        // Always set Content-Length
                    mg_write(conn, in_bfr,iReadLen );
                    return true;
                } else{
                    Debug::out(LOG_ERROR, "could not send config command");
                    mg_printf(conn, "HTTP/1.1 500 Server Error - could not send config command\r\n\r\n");
                    return true;
                }                
            } else{
                Debug::out(LOG_ERROR, "json malformed");
                mg_printf(conn, "HTTP/1.1 400 Bad Request - json malformed\r\n\r\n");
                return true;
            }
        } 
    
        mg_printf(conn, "HTTP/1.1 200 OK\r\n\r\n");
        return true;    
    }

    return false;
}

int ConfigResource::sendTerminalCommand(unsigned char cmd, unsigned char param)
{
    char bfr[3];
    int  res, bytesAvailable;
    
    openFIFOsIfNeeded();
    
    if(webFd1 <= 0 || webFd2 <= 0) {
        Debug::out(LOG_DEBUG, "ConfigResource::sendTerminalCommand() -- FIFOs not open");
        return -1;
    }

    //------------
    // first check if there isn't something stuck in the fifo, and if there is, just read it to discard it
    while(1) {
        res = ioctl(webFd2, FIONREAD, &bytesAvailable); // how many bytes we can read?

        if(res != -1 && bytesAvailable > 0) {           // ioctl success and something to read? read it, ignore it
            read(webFd2, in_bfr, bytesAvailable);
        } else {                                        // nothing more to read, quit this loop and continue with the rest
            break;
        }
    }
    //------------
    
    bfr[0] = HOSTMOD_CONFIG;
    bfr[1] = cmd;
    bfr[2] = param;
    
    res = write(webFd1, bfr, 3);
    
    if(res != 3) {
        Debug::out(LOG_DEBUG, "ConfigResource::sendTerminalCommand() -- sendCmd - write failed!");
        return -1;
    }
    
    DWORD timeOutTime = Utils::getEndTime(1000);

    while(1) {                                          // wait up to 1 second to receive something... 
        if(Utils::getCurrentMs() >= timeOutTime) {      // time out happened, nothing received within specified timeout? fail
            return -1;
        }
    
        res = ioctl(webFd2, FIONREAD, &bytesAvailable);                         // how many bytes we can read?

        if(res != -1 && bytesAvailable > 0) {                                   // ioctl success and something to read?
            int readCount = (bytesAvailable < INBFR_SIZE) ? bytesAvailable : INBFR_SIZE;

            memset(in_bfr, 0, readCount + 1);
            read(webFd2, in_bfr, readCount);                                    // get the stream

            Debug::out(LOG_DEBUG, "sendCmd - readCount: %d", readCount);

            int newCount = translateVT52toVT100(in_bfr, tmp_bfr, readCount);    // translate VT52 stream to VT100 stream
            return newCount;
        }
        
        Utils::sleepMs(50);     // sleep a little
    }
}
