#include <stdio.h>#include <stdlib.h>#include <string.h>#include "../debug.h"#include "netsettings.h"#define NETWORK_CONFIG_FILE		"/etc/network/interfaces"#define NAMESERVER_FILE			"/etc/resolv.conf"#define WPA_SUPPLICANT_FILE     "/etc/wpa_supplicant.conf"NetworkSettings::NetworkSettings(void){	initNetSettings(&eth0);	initNetSettings(&wlan0);		nameserver = "";}void NetworkSettings::initNetSettings(TNetInterface *neti){	neti->dhcpNotStatic	= true;	neti->address		= "";    neti->netmask		= "";    neti->gateway		= "";		neti->wpaSsid		= "";	neti->wpaPsk		= "";}void NetworkSettings::load(void){	FILE *f = fopen(NETWORK_CONFIG_FILE, "rt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::load - failed to open network settings file.\n");		return;	}		initNetSettings(&eth0);	initNetSettings(&wlan0);		#define MAX_LINE_LEN	1024	char line[MAX_LINE_LEN];		TNetInterface *currentIface = NULL;							// store the settings to the struct pointed by this pointer		while(!feof(f)) {		char *res = fgets(line, MAX_LINE_LEN, f);				// get single line				if(!res) {												// if failed to get the line			break;		}				// found start of iface section?		if(strstr(line, "iface") != NULL) {			if(strstr(line, "eth0") != NULL) {					// found eth0 section?				currentIface = &eth0;				initNetSettings(currentIface);					// clear the struct			}			if(strstr(line, "wlan0") != NULL) {					// found wlan0 section?				currentIface = &wlan0;				initNetSettings(currentIface);					// clear the struct			}					if(!currentIface) {									// it wasn't eth0 and it wasn't wlan0?				continue;			}					if(strstr(line, "inet dhcp") != NULL) {				// dhcp config?				currentIface->dhcpNotStatic = true;			}			if(strstr(line, "inet static") != NULL) {			// static config?				currentIface->dhcpNotStatic = false;			}		}		if(!currentIface) {										// current interface not (yet) set? skip the rest			continue;		}		readString(line, (char *) "address",	currentIface->address);		readString(line, (char *) "netmask",	currentIface->netmask);		readString(line, (char *) "gateway",	currentIface->gateway);	}		fclose(f);    loadWpaSupplicant();	loadNameserver();	dumpSettings();}void NetworkSettings::readString(char *line, char *tag, std::string &val){	char *str = strstr(line, tag);					// find tag position	if(str == NULL) {									// tag not present?		return;	}		int tagLen = strlen(tag);							// get tag length		char tmp[1024];	int ires = sscanf(str + tagLen + 1, "%s", tmp);		// read the value			if(ires != 1) {										// reading value failed?		return;	}	val = tmp;											// store value	if(val.length() < 1) {		return;	}		if(val.at(0) == '"') {								// starts with double quotes? remove them		val.erase(0, 1);	}		size_t pos = val.rfind("\"");							// find last occurence or double quotes	if(pos != std::string::npos) {						// erase last double quotes		val.erase(pos, 1);	}	}void NetworkSettings::dumpSettings(void){	Debug::out(LOG_INFO, (char *) "Network settings\n");	Debug::out(LOG_INFO, (char *) "eth0:\n");	Debug::out(LOG_INFO, (char *) "      DHCP %d\n", eth0.dhcpNotStatic);	Debug::out(LOG_INFO, (char *) "   address %s\n", (char *) eth0.address.c_str());	Debug::out(LOG_INFO, (char *) "   netmask %s\n", (char *) eth0.netmask.c_str());	Debug::out(LOG_INFO, (char *) "   gateway %s\n", (char *) eth0.gateway.c_str());	Debug::out(LOG_INFO, (char *) "\nwlan0:\n");	Debug::out(LOG_INFO, (char *) "      DHCP %d\n", wlan0.dhcpNotStatic);	Debug::out(LOG_INFO, (char *) "   address %s\n", (char *) wlan0.address.c_str());	Debug::out(LOG_INFO, (char *) "   netmask %s\n", (char *) wlan0.netmask.c_str());	Debug::out(LOG_INFO, (char *) "   gateway %s\n", (char *) wlan0.gateway.c_str());	Debug::out(LOG_INFO, (char *) "  wpa-ssid %s\n", (char *) wlan0.wpaSsid.c_str());	Debug::out(LOG_INFO, (char *) "   wpa-psk %s\n", (char *) wlan0.wpaPsk.c_str());		Debug::out(LOG_INFO, (char *) "\nnameserver %s\n", (char *) nameserver.c_str());}void NetworkSettings::save(void){	FILE *f = fopen(NETWORK_CONFIG_FILE, "wt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::save - failed to open network settings file.\n");		return;	}		// lo section	fprintf(f, "# The loopback network interface\n");	fprintf(f, "auto lo\n");	fprintf(f, "iface lo inet loopback\n\n");		// eth section	fprintf(f, "# The primary network interface\n");    writeNetInterfaceSettings(f, eth0, (char *) "eth0");		// wlan section	fprintf(f, "# The wireless network interface\n");    writeNetInterfaceSettings(f, wlan0, (char *) "wlan0");  	fprintf(f, "wpa-conf %s \n\n", WPA_SUPPLICANT_FILE);		fclose(f);	    saveWpaSupplicant();	saveNameserver();}void NetworkSettings::writeNetInterfaceSettings(FILE *f, TNetInterface &iface, char *ifaceName){	fprintf(f, "auto %s\n", ifaceName);	fprintf(f, "iface %s inet ", ifaceName);		if(iface.dhcpNotStatic) {		fprintf(f, "dhcp\n");	} else {		fprintf(f, "static\n");		    fprintf(f, "address %s\n", (char *) iface.address.c_str());		fprintf(f, "netmask %s\n", (char *) iface.netmask.c_str()); 		fprintf(f, "gateway %s\n", (char *) iface.gateway.c_str());	}    	fprintf(f, "\n");}void NetworkSettings::saveWpaSupplicant(void){	FILE *f = fopen(WPA_SUPPLICANT_FILE, "wt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::saveWpaSupplicant - failed to open wpa supplication file.\n");		return;	}        fprintf(f, "network={\n");	fprintf(f, "    ssid \"%s\"\n",	(char *) wlan0.wpaSsid.c_str()); 	fprintf(f, "    psk \"%s\"\n",	(char *) wlan0.wpaPsk.c_str());    fprintf(f, "}\n\n");    fclose(f);}void NetworkSettings::loadWpaSupplicant(void){	FILE *f = fopen(WPA_SUPPLICANT_FILE, "rt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::loadWpaSupplicant - failed to open wpa supplicant file, this might be OK\n");		return;	}		#define MAX_LINE_LEN	1024	char line[MAX_LINE_LEN];		while(!feof(f)) {		char *res = fgets(line, MAX_LINE_LEN, f);				// get single line				if(!res) {												// if failed to get the line			break;		}		        char *p;                p = strstr(line, "ssid");                               // it's a line with SSID?        if(p != NULL) {            readString(line, (char *) "ssid", wlan0.wpaSsid);            continue;        }                p = strstr(line, "psk");                                // it's a line with PSK?        if(p != NULL) {            readString(line, (char *) "psk", wlan0.wpaPsk);            continue;        }    }        fclose(f);}void NetworkSettings::loadNameserver(void){	FILE *f = fopen(NAMESERVER_FILE, "rt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::loadNameserver - failed to open settings file.\n");		return;	}		char tmp[1024];	int ires = fscanf(f, "nameserver %s\n", tmp);		if(ires == 1) {		nameserver = tmp;	}		fclose(f);}void NetworkSettings::saveNameserver(void){	FILE *f = fopen(NAMESERVER_FILE, "wt");		if(!f) {		Debug::out(LOG_ERROR, (char *) "NetworkSettings::saveNameserver - failed to open settings file.\n");		return;	}		fprintf(f, "nameserver %s\n", (char *) nameserver.c_str());	fclose(f);}		