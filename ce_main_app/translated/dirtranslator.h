#ifndef DIRTRANSLATOR_H
#define DIRTRANSLATOR_H

#include <iostream>
#include <map>

#include "../datatypes.h"

class FilenameShortener;

class TFindStorage {
public:
    TFindStorage();
    ~TFindStorage();

    int getSize(void);
    void clear(void);
    void copyDataFromOther(TFindStorage *other);

    DWORD dta;

    BYTE *buffer;
    WORD count;             // count of items found

    WORD maxCount;          // maximum count of items that this buffer can hold
};

class DirTranslator
{
public:
    DirTranslator();
    ~DirTranslator();

    // clear all the maps
    void clear(void);

    // convert single filename from long to short
    bool longToShortFilename(const std::string &longHostPath, const std::string &longFname, std::string &shortFname);

    // convert whole path from short to long
    void shortToLongPath(const std::string &rootPath, const std::string &shortPath, std::string &longPath);    // convert 'long_p~1\\sub_fo~1\\anothe~1' to 'long path/sub folder/another one'

    // call this for find first / find next for Gemdos
    bool buildGemdosFindstorageData(TFindStorage *fs, std::string hostSearchPathAndWildcards, BYTE findAttribs, bool isRootDir, bool useZipdirNotFile);

    static void toUpperCaseString(std::string &st);

private:
    std::map<std::string, FilenameShortener *>  mapPathToShortener;
    
    TFindStorage    fsDirs;
    TFindStorage    fsFiles;
    
    FilenameShortener *getShortenerForPath(std::string path);
    FilenameShortener *createShortener(const std::string &path);
    static void splitFilenameFromPath(std::string &pathAndFile, std::string &path, std::string &file);

    void appendFoundToFindStorage(std::string &hostPath, const char *searchString, TFindStorage *fs, struct dirent *de, BYTE findAttribs);
    void appendFoundToFindStorage(std::string &hostPath, const char *searchString, TFindStorage *fs, const char *name, bool isDir, BYTE findAttribs);
    void appendFoundToFindStorage_dirUpDirCurr(std::string &hostPath, const char *searchString, TFindStorage *fs, struct dirent *de, BYTE findAttribs);

    static int compareSearchStringAndFilename(const char *searchString, const char *filename);
};

#endif // DIRTRANSLATOR_H
