// vim: expandtab shiftwidth=4 tabstop=4
#ifndef SETTINGSRELOADPROXY_H
#define SETTINGSRELOADPROXY_H

#include "isettingsuser.h"

#define SETTINGSUSER_NONE           0
#define SETTINGSUSER_ACSI           1
#define SETTINGSUSER_TRANSLATED     2
#define SETTINGSUSER_SHARED         3
#define SETTINGSUSER_FLOPPYCONF     4
#define SETTINGSUSER_FLOPPYIMGS     5
#define SETTINGSUSER_FLOPPY_SLOT    6
#define SETTINGSUSER_SCSI_IDS       7

#define MAX_SETTINGS_USERS      16

typedef struct {
    ISettingsUser   *su;
    int             type;
} TSettUsr;

class SettingsReloadProxy
{
public:
    SettingsReloadProxy();

    void addSettingsUser(ISettingsUser *su, int type);
    void reloadSettings(int type);

private:
    TSettUsr settUser[MAX_SETTINGS_USERS];

};

#endif // SETTINGSRELOADPROXY_H
