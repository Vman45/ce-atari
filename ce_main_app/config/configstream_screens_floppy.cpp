#include <stdio.h>
#include <string.h>

#include "../global.h"
#include "../debug.h"
#include "../native/scsi_defs.h"
#include "../acsidatatrans.h"
#include "../mounter.h"
#include "../display/displaythread.h"

#include "../settings.h"
#include "../utils.h"
#include "keys.h"
#include "configstream.h"
#include "netsettings.h"

//--------------------------
// screen creation methods
void ConfigStream::createScreen_floppy_config(void)
{
    // the following 3 lines should be at start of each createScreen_ method
    destroyCurrentScreen();			    // destroy current components
    screenChanged	    = true;			// mark that the screen has changed
    showingHomeScreen	= false;		// mark that we're NOT showing the home screen

    screen_addHeaderAndFooter(screen, (char *) "Floppy configuration");

    ConfigComponent *comp;

    int row = 6;
    int col1 = 5, col2 = 22, col3 = 30;

    comp = new ConfigComponent(this, ConfigComponent::label, "Floppy enabled", 15, col1, row, gotoOffset);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::checkbox, "   ", 3, col2, row, gotoOffset);
    comp->setComponentId(COMPID_FLOPCONF_ENABLED);
    screen.push_back(comp);

    row += 2;

    comp = new ConfigComponent(this, ConfigComponent::label, "0", 3, col2 + 2, row, gotoOffset);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::label, "1", 3, col3 + 2, row, gotoOffset);
    screen.push_back(comp);

    row++;

    comp = new ConfigComponent(this, ConfigComponent::label, "Drive ID", 15, col1, row, gotoOffset);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::checkbox, "   ", 3, col2, row, gotoOffset);
    comp->setCheckboxGroupIds(COMPID_FLOPCONF_ID, 0);																// set checkbox group id to COMPID_FLOPCONF_ID, and checbox id to drive ID
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::checkbox, "   ", 3, col3, row, gotoOffset);
    comp->setCheckboxGroupIds(COMPID_FLOPCONF_ID, 1);																// set checkbox group id to COMPID_FLOPCONF_ID, and checbox id to drive ID
    screen.push_back(comp);

    row += 3;

    comp = new ConfigComponent(this, ConfigComponent::label, "Write protected", 15, col1, row, gotoOffset);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::checkbox, "   ", 3, col2, row, gotoOffset);
    comp->setComponentId(COMPID_FLOPCONF_WRPROT);
    screen.push_back(comp);

    row += 3;

    comp = new ConfigComponent(this, ConfigComponent::label, "Make seek sound", 15, col1, row, gotoOffset);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::checkbox, "   ", 3, col2, row, gotoOffset);
    comp->setComponentId(COMPID_FLOPSOUND_ENABLED);
    screen.push_back(comp);

    row = 20;

    comp = new ConfigComponent(this, ConfigComponent::button, " Save ", 6,  9, row, gotoOffset);
    comp->setOnEnterFunctionCode(CS_FLOPPY_CONFIG_SAVE);
    comp->setComponentId(COMPID_BTN_SAVE);
    screen.push_back(comp);

    comp = new ConfigComponent(this, ConfigComponent::button, " Cancel ", 8, 20, row, gotoOffset);
    comp->setOnEnterFunctionCode(CS_GO_HOME);
    comp->setComponentId(COMPID_BTN_CANCEL);
    screen.push_back(comp);

    Settings        s;
    FloppyConfig    fc;

    s.loadFloppyConfig(&fc);

    setBoolByComponentId(COMPID_FLOPCONF_ENABLED,   fc.enabled);
    checkboxGroup_setCheckedId(COMPID_FLOPCONF_ID,  fc.id);
    setBoolByComponentId(COMPID_FLOPCONF_WRPROT,    fc.writeProtected);
    setBoolByComponentId(COMPID_FLOPSOUND_ENABLED,  fc.soundEnabled);

    setFocusToFirstFocusable();
}

void ConfigStream::onFloppyConfigSave(void)
{
    Settings        s;
    FloppyConfig    fc;

    // retrieve settings from screen components
    getBoolByComponentId(COMPID_FLOPCONF_ENABLED,   fc.enabled);
    getBoolByComponentId(COMPID_FLOPCONF_WRPROT,    fc.writeProtected);
    fc.id = checkboxGroup_getCheckedId(COMPID_FLOPCONF_ID);
    getBoolByComponentId(COMPID_FLOPSOUND_ENABLED,  fc.soundEnabled);

    // store them to the settings files
    s.saveFloppyConfig(&fc);

    // if got settings reload proxy, invoke reload
    if(reloadProxy) {
        reloadProxy->reloadSettings(SETTINGSUSER_FLOPPYCONF);
    }

    // tell the beeper thread to reload settings
    beeper_reloadSettings();

    // now back to the home screen
    createScreen_homeScreen();
}

