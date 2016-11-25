#include <mint/sysbind.h>
#include <mint/osbind.h>
#include <mint/ostruct.h>
#include <support.h>

#include <stdio.h>
#include "stdlib.h"
#include "out.h"

int   bufferSize;
char *buffer;

char *bfrCur;
int   curSize;

void outString(char *str);

void fixTestName(char *before, char *after)
{
    int len = strlen(before);

    memset(after, ' ', 40);
    after[40] = 0;
    
    if(len <= 40) {
        memcpy(after, before, len);
    } else {
        memcpy(after, before, 40);
    }
}

char *resultToString(BYTE result)
{
    static char *testResults[2] = {"[ OK ]", "[fail]"};
    char *resStr = result ? testResults[0] : testResults[1];

    return resStr;
}

void out_string_lenght(char *str, int len)
{
    int strLen = strlen(str);
    
    char tmp[100];
    memset(tmp, ' ', len);
    tmp[len]    = 0;
    
    strncpy(tmp, str, (strLen < len) ? strLen : len);
    outString(tmp);
}

void out_test_header(WORD testNo, char *testName)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    outString("[");
    outString(testNoStr);
    outString("] [");
    out_string_lenght(testName, 40);
    outString("]");
}

void out_result(BYTE result)
{
    char *resultString = resultToString(result);
    outString(resultString);
    outString("\n\r");
}

void out_result_error(BYTE result, WORD errorCode)
{
    char *resultString = resultToString(result);
    outString(resultString);
    
    char errCodeStr[16];
    wordToHex(errorCode, errCodeStr);
    outString(" [");
    outString(errCodeStr);
    outString("]\n\r");
}

void out_result_error_string(BYTE result, WORD errorCode, char *errorStr)
{
    char *resultString = resultToString(result);
    outString(resultString);
    
    char errCodeStr[16];
    wordToHex(errorCode, errCodeStr);
    outString(" [");
    outString(errCodeStr);
    outString("] err: ");
    
    outString(errorStr);
    outString("\n\r");
}

void out_result_string(BYTE result, char *errorStr)
{
    char *resultString = resultToString(result);
    outString(resultString);
    
    outString(" err: ");
    outString(errorStr);
    outString("\n\r");
}

void out_result_string_dw_w(BYTE result, char *errorStr, DWORD dw, WORD w)
{
    char *resultString = resultToString(result);
    outString(resultString);
    
    outString(" err: ");
    outString(errorStr);

    char tmp[16];
    
    outString (" ");
    dwordToHex(dw, tmp);
    outString (tmp);

    outString (" ");
    wordToHex(w, tmp);
    outString (tmp);

    outString("\n\r");
}

// output Test Result - bool
void out_tr_b(WORD testNo, char *testName, BYTE result)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    char testNameFixed[41];
    fixTestName(testName, testNameFixed);
    
    char *resultString = resultToString(result);
    
    outString("[");
    outString(testNoStr);
    outString("] [");
    outString(testNameFixed);
    outString("] ");
    outString(resultString);
    outString("\n\r");
}

// output Test Result - bool, word
void out_tr_bw(WORD testNo, char *testName, BYTE result, WORD errorCode)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    char testNameFixed[41];
    fixTestName(testName, testNameFixed);
    
    char *resultString = resultToString(result);
    
    char errCodeStr[16];
    wordToHex(errorCode, errCodeStr);
    
    outString("[");
    outString(testNoStr);
    outString("] [");
    outString(testNameFixed);
    outString("] ");
    outString(resultString);
    outString(" [");
    outString(errCodeStr);
    outString("]\n\r");
}

// output Test Result - test name, error string, bool result, word errorcode
void out_tr_ebw(WORD testNo, char *testName, char *errorString, BYTE result, WORD errorCode)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    //-----------
    // create string, which will contain both test name and also error string
    char testNameWithErrorString[256];
    strcpy(testNameWithErrorString, testName);
    strcat(testNameWithErrorString, " - ");
    strcat(testNameWithErrorString, errorString);
    //-----------
    
    char testNameFixed[41];
    fixTestName(testNameWithErrorString, testNameFixed);
    
    char *resultString = resultToString(result);
    
    char errCodeStr[16];
    wordToHex(errorCode, errCodeStr);
    
    outString("[");
    outString(testNoStr);
    outString("] [");
    outString(testNameFixed);
    outString("] ");
    outString(resultString);
    outString(" [");
    outString(errCodeStr);
    outString("]\n\r");
}

// output Test Result - test name, error string, bool result
void out_tr_eb(WORD testNo, char *testName, char *errorString, BYTE result)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    //-----------
    // create string, which will contain both test name and also error string
    char testNameWithErrorString[256];
    strcpy(testNameWithErrorString, testName);
    strcat(testNameWithErrorString, " - ");
    strcat(testNameWithErrorString, errorString);
    //-----------
    
    char testNameFixed[41];
    fixTestName(testNameWithErrorString, testNameFixed);
    
    char *resultString = resultToString(result);
    
    outString("[");
    outString(testNoStr);
    outString("] [");
    outString(testNameFixed);
    outString("] ");
    outString(resultString);
    outString("\n\r");
}

// output Test Result - bool, dword
void out_tr_bd(WORD testNo, char *testName, BYTE result, DWORD errorCode)
{
    char testNoStr[16];
    wordToHex(testNo, testNoStr);

    char testNameFixed[41];
    fixTestName(testName, testNameFixed);
    
    char *resultString = resultToString(result);
    
    char errCodeStr[16];
    dwordToHex(errorCode, errCodeStr);
    
    outString("[");
    outString(testNoStr);
    outString("] [");
    outString(testNameFixed);
    outString("] ");
    outString(resultString);
    outString(" [");
    outString(errCodeStr);
    outString("]\n\r");
}

void out_swsw(char *str1, WORD w1, char *str2, WORD w2)
{
    char tmp[16];
    
    outString(str1);
    wordToHex(w1, tmp);
    outString(tmp);

    outString(str2);
    wordToHex(w2, tmp);
    outString(tmp);

    outString("\n\r");
}

void out_s(char *str1)
{
    outString(str1);
    outString("\n\r");
}

void out_ss(char *str1, char *str2)
{
    outString(str1);
    outString(str2);
    outString("\n\r");
}

void out_sw(char *str1, WORD w1)
{
    char tmp[16];
    
    wordToHex(w1, tmp);
    
    outString(str1);
    outString(tmp);
    outString("\n\r");
}

void out_sc(char *str1, char c)
{
    char tmp[16];

    tmp[0] = c;
    tmp[1] = 0;
    
    outString(str1);
    outString(tmp);
    outString("\n\r");
}

void outString(char *str)
{
    (void) Cconws(str);         // to screen
    appendToBuf(str);           // to buffer
}

void appendToBuf(char *str)
{
    if(!bfrCur) {                               // no buffer? quit
        return;
    }

    int len = strlen(str);
    
    if((curSize + len + 1) > bufferSize) {      // would go out of buffer? quit
        return;
    }
    
    strcpy_switch_rn(bfrCur, str);              // copy in the data
    
    bfrCur  += len;                             // move the pointer, update the counter
    curSize += len;
}

#define BUFFER_SIZE     (64 * 1024)
void initBuffer(void)
{
    bufferSize  = 0;
    buffer      = NULL;

    curSize     = 0;
    bfrCur      = NULL;

    buffer      = (char *) Malloc(BUFFER_SIZE); // try to malloc
    
    if(buffer != NULL) {                        // on success - store size and pointer
        bufferSize  = BUFFER_SIZE;
        bfrCur      = buffer;
    } else {
        (void) Cconws("Failed to allocate buffer for results!\r\n");
    }
}

void writeBufferToFile(void)
{
    if(!buffer) {
        return;
    }

    char path[32] = {"X:\\STNG_TST.txt"};       // filename string template
    
    char fromDrive = Dgetdrv();
    path[0] = 'A' + fromDrive;                  // place drive letter - of drive where the test was runned from 
    int f = Fcreate(path, 0);                   // create file
    
    if(f < 0) {
        (void) Cconws("\r\nFailed to create log file.\r\n");
        return;
    } 
    
    (void) Cconws("\r\n\33pSaved log file: ");
    (void) Cconws(path);
    (void) Cconws(" \33q\r\n");
    
    Fwrite(f, curSize, buffer);
    Fclose(f);
}

void deinitBuffer(void)
{
    if(!buffer) {
        return;
    }
    
    Mfree(buffer);
    
    buffer      = NULL;
    bfrCur      = NULL;
    bufferSize  = 0;
    curSize     = 0;
}

void byteToHex( BYTE val, char *bfr)
{
    int hi, lo;
    char table[16] = {"0123456789ABCDEF"};
    
    hi = (val >> 4) & 0x0f;;
    lo = (val     ) & 0x0f;

    bfr[0] = table[hi];
    bfr[1] = table[lo];
    bfr[2] = 0;
}

void wordToHex( WORD val, char *bfr)
{
    BYTE a,b;
    a = val >>  8;
    b = val;
    
    byteToHex(a, bfr + 0);
    byteToHex(b, bfr + 2);
}

void dwordToHex(DWORD val, char *bfr)
{
    BYTE a,b,c,d;
    a = val >> 24;
    b = val >> 16;
    c = val >>  8;
    d = val;
    
    byteToHex(a, bfr + 0);
    byteToHex(b, bfr + 2);
    byteToHex(c, bfr + 4);
    byteToHex(d, bfr + 6);    
}
