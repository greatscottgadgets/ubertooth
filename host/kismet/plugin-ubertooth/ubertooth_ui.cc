/* -*- c++ -*- */
/*
 * Copyright 2010, 2011 Michael Ossmann
 * Copyright 2009, 2010 Mike Kershaw
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <stdio.h>

#include <string>
#include <sstream>

#include <globalregistry.h>
#include <gpscore.h>
#include <kis_panel_plugin.h>
#include <kis_panel_frontend.h>
#include <kis_panel_windows.h>
#include <kis_panel_network.h>
#include <kis_panel_widgets.h>
#include <version.h>

#include "tracker_btbb.h"

const char *btbbdev_fields[] = {
	"lap", "bdaddr", "firsttime",
	"lasttime", "packets",
	GPS_COMMON_FIELDS_TEXT,
	NULL
};

enum btbb_sort_type {
	btbb_sort_bdaddr,
	btbb_sort_firsttime,
	btbb_sort_lasttime,
	btbb_sort_packets
};

struct ubertooth_data {
	int mi_plugin_ubertooth, mi_showubertooth;

	int mn_sub_sort, mi_sort_bdaddr, mi_sort_firsttime,
			mi_sort_lasttime, mi_sort_packets;

	map<uint32_t, btbb_network *> btdev_map;
	vector<btbb_network *> btdev_vec;

	Kis_Scrollable_Table *btdevlist;

	int cliaddref;

	int timerid;

	string asm_btbbdev_fields;
	int asm_btbbdev_num;

	btbb_sort_type sort_type;

	KisPanelPluginData *pdata;
	Kis_Menu *menu;
};

class Ubertooth_Sort_Bdaddr {
public:
	inline bool operator()(btbb_network *x, btbb_network *y) const {
		return x->bd_addr < y->bd_addr;
	}
};

class Ubertooth_Sort_Firsttime {
public:
	inline bool operator()(btbb_network *x, btbb_network *y) const {
		return x->first_time < y->first_time;
	}
};

class Ubertooth_Sort_Lasttime {
public:
	inline bool operator()(btbb_network *x, btbb_network *y) const {
		return x->last_time < y->last_time;
	}
};

class Ubertooth_Sort_Packets {
public:
	inline bool operator()(btbb_network *x, btbb_network *y) const {
		return x->num_packets < y->num_packets;
	}
};

// Menu events
int Ubertooth_plugin_menu_cb(void *auxptr);
void Ubertooth_show_menu_cb(MENUITEM_CB_PARMS);
void Ubertooth_sort_menu_cb(MENUITEM_CB_PARMS);

// Network events
void UbertoothCliAdd(KPI_ADDCLI_CB_PARMS);
void UbertoothCliConfigured(CLICONF_CB_PARMS);

// List select
int UbertoothDevlistCB(COMPONENT_CALLBACK_PARMS);

// List content timer
int UbertoothTimer(TIMEEVENT_PARMS);

// Details panel
class Ubertooth_Details_Panel : public Kis_Panel {
public:
	Ubertooth_Details_Panel() {
		fprintf(stderr, "FATAL OOPS: Ubertooth_Details_Panel()\n");
		exit(1);
	}

	Ubertooth_Details_Panel(GlobalRegistry *in_globalreg, KisPanelInterface *in_kpf);
	virtual ~Ubertooth_Details_Panel();

	virtual void DrawPanel();
	virtual void ButtonAction(Kis_Panel_Component *in_button);
	virtual void MenuAction(int opt);

	void SetUbertooth(ubertooth_data *in_bt) { ubertooth = in_bt; }
	void SetDetailsNet(btbb_network *in_net) { btnet = in_net; }

protected:
	ubertooth_data *ubertooth;
	btbb_network *btnet;

	Kis_Panel_Packbox *vbox;
	Kis_Free_Text *btdetailt;

	Kis_Button *closebutton;
};

extern "C" {

int panel_plugin_init(GlobalRegistry *globalreg, KisPanelPluginData *pdata) {
	_MSG("Loading Kismet Ubertooth plugin", MSGFLAG_INFO);

	ubertooth_data *ubertooth = new ubertooth_data;

	pdata->pluginaux = (void *) ubertooth;

	ubertooth->pdata = pdata;

	ubertooth->sort_type = btbb_sort_bdaddr;

	ubertooth->asm_btbbdev_num = 
		TokenNullJoin(&(ubertooth->asm_btbbdev_fields), btbbdev_fields);

	ubertooth->mi_plugin_ubertooth =
		pdata->mainpanel->AddPluginMenuItem("Ubertooth", Ubertooth_plugin_menu_cb, pdata);

	ubertooth->btdevlist = new Kis_Scrollable_Table(globalreg, pdata->mainpanel);

	vector<Kis_Scrollable_Table::title_data> titles;
	Kis_Scrollable_Table::title_data t;

	t.width = 17;
	t.title = "BD_ADDR";
	t.alignment = 0;
	titles.push_back(t);

	t.width = 5;
	t.title = "Count";
	t.alignment = 2;
	titles.push_back(t);

	ubertooth->btdevlist->AddTitles(titles);
	ubertooth->btdevlist->SetPreferredSize(0, 10);

	ubertooth->btdevlist->SetHighlightSelected(1);
	ubertooth->btdevlist->SetLockScrollTop(1);
	ubertooth->btdevlist->SetDrawTitles(1);

	ubertooth->btdevlist->SetCallback(COMPONENT_CBTYPE_ACTIVATED, UbertoothDevlistCB, ubertooth);

	pdata->mainpanel->AddComponentVec(ubertooth->btdevlist, 
									  (KIS_PANEL_COMP_DRAW | KIS_PANEL_COMP_EVT |
									   KIS_PANEL_COMP_TAB));
	pdata->mainpanel->FetchNetBox()->Pack_After_Named("KIS_MAIN_NETLIST",
													  ubertooth->btdevlist, 1, 0);

	ubertooth->menu = pdata->kpinterface->FetchMainPanel()->FetchMenu();
	int mn_view = ubertooth->menu->FindMenu("View");

	pdata->kpinterface->FetchMainPanel()->AddViewSeparator();
	ubertooth->mi_showubertooth = ubertooth->menu->AddMenuItem("Ubertooth", mn_view, 0);
	ubertooth->menu->SetMenuItemCallback(ubertooth->mi_showubertooth, Ubertooth_show_menu_cb, 
									  ubertooth);

	pdata->kpinterface->FetchMainPanel()->AddSortSeparator();
	int mn_sort = ubertooth->menu->FindMenu("Sort");
	ubertooth->mn_sub_sort = ubertooth->menu->AddSubMenuItem("Ubertooth", mn_sort, 0);
	ubertooth->mi_sort_bdaddr = 
		ubertooth->menu->AddMenuItem("BD_ADDR", ubertooth->mn_sub_sort, 0);
	ubertooth->mi_sort_firsttime = 
		ubertooth->menu->AddMenuItem("First Time", ubertooth->mn_sub_sort, 0);
	ubertooth->mi_sort_lasttime = 
		ubertooth->menu->AddMenuItem("Last Time", ubertooth->mn_sub_sort, 0);
	ubertooth->mi_sort_packets = 
		ubertooth->menu->AddMenuItem("Times Seen", ubertooth->mn_sub_sort, 0);

	ubertooth->menu->SetMenuItemCallback(ubertooth->mi_sort_bdaddr, Ubertooth_sort_menu_cb, 
									  ubertooth);
	ubertooth->menu->SetMenuItemCallback(ubertooth->mi_sort_firsttime, Ubertooth_sort_menu_cb, 
									  ubertooth);
	ubertooth->menu->SetMenuItemCallback(ubertooth->mi_sort_lasttime, Ubertooth_sort_menu_cb, 
									  ubertooth);
	ubertooth->menu->SetMenuItemCallback(ubertooth->mi_sort_packets, Ubertooth_sort_menu_cb, 
									  ubertooth);

	string opt = StrLower(pdata->kpinterface->prefs->FetchOpt("PLUGIN_UBERTOOTH_SHOW"));
	if (opt == "true" || opt == "") {
		ubertooth->btdevlist->Show();
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_showubertooth, 1);

		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_bdaddr);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_firsttime);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_lasttime);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_packets);

	} else {
		ubertooth->btdevlist->Hide();
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_showubertooth, 0);

		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_bdaddr);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_firsttime);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_lasttime);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_packets);
	}

	opt = pdata->kpinterface->prefs->FetchOpt("PLUGIN_UBERTOOTH_SORT");
	if (opt == "bdaddr") {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_bdaddr, 1);
		ubertooth->sort_type = btbb_sort_bdaddr;
	} else if (opt == "firsttime") {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_firsttime, 1);
		ubertooth->sort_type = btbb_sort_firsttime;
	} else if (opt == "lasttime") {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_lasttime, 1);
		ubertooth->sort_type = btbb_sort_lasttime;
	} else if (opt == "packets") {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_packets, 1);
		ubertooth->sort_type = btbb_sort_packets;
	} else {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_bdaddr, 1);
		ubertooth->sort_type = btbb_sort_bdaddr;
	}

	// Register the timer event for populating the array
	ubertooth->timerid = 
		globalreg->timetracker->RegisterTimer(SERVER_TIMESLICES_SEC, NULL,
											  1, &UbertoothTimer, ubertooth);

	// Do this LAST.  The configure event is responsible for clearing out the
	// list on reconnect, but MAY be called immediately upon being registered
	// if the client is already valid.  We have to have made all the other
	// bits first before it's valid to call this
	ubertooth->cliaddref =
		pdata->kpinterface->Add_NetCli_AddCli_CB(UbertoothCliAdd, (void *) ubertooth);

	return 1;
}

// Plugin version control
void kis_revision_info(panel_plugin_revision *prev) {
	if (prev->version_api_revision >= 1) {
		prev->version_api_revision = 1;
		prev->major = string(VERSION_MAJOR);
		prev->minor = string(VERSION_MINOR);
		prev->tiny = string(VERSION_TINY);
	}
}

}

int Ubertooth_plugin_menu_cb(void *auxptr) {
	KisPanelPluginData *pdata = (KisPanelPluginData *) auxptr;

	pdata->kpinterface->RaiseAlert("Ubertooth",
			"Ubertooth UI " + string(VERSION_MAJOR) + "-" + string(VERSION_MINOR) + "-" +
				string(VERSION_TINY) + "\n"
			"\n"
			"Display Bluetooth/802.15.1 devices found by the\n"
			"Kismet Ubertooth plugin\n");
	return 1;
}

void Ubertooth_show_menu_cb(MENUITEM_CB_PARMS) {
	ubertooth_data *ubertooth = (ubertooth_data *) auxptr;

	if (ubertooth->pdata->kpinterface->prefs->FetchOpt("PLUGIN_UBERTOOTH_SHOW") == "true" ||
		ubertooth->pdata->kpinterface->prefs->FetchOpt("PLUGIN_UBERTOOTH_SHOW") == "") {

		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SHOW", "false", 1);

		ubertooth->btdevlist->Hide();

		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_bdaddr);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_firsttime);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_lasttime);
		ubertooth->menu->DisableMenuItem(ubertooth->mi_sort_packets);

		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_showubertooth, 0);
	} else {
		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SHOW", "true", 1);

		ubertooth->btdevlist->Show();

		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_bdaddr);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_firsttime);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_lasttime);
		ubertooth->menu->EnableMenuItem(ubertooth->mi_sort_packets);

		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_showubertooth, 1);
	}

	return;
}

void Ubertooth_sort_menu_cb(MENUITEM_CB_PARMS) {
	ubertooth_data *ubertooth = (ubertooth_data *) auxptr;

	ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_bdaddr, 0);
	ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_firsttime, 0);
	ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_lasttime, 0);
	ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_packets, 0);

	if (menuitem == ubertooth->mi_sort_bdaddr) {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_bdaddr, 1);
		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SORT", "bdaddr", 
												  globalreg->timestamp.tv_sec);
		ubertooth->sort_type = btbb_sort_bdaddr;
	} else if (menuitem == ubertooth->mi_sort_firsttime) {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_firsttime, 1);
		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SORT", "firsttime", 
												  globalreg->timestamp.tv_sec);
		ubertooth->sort_type = btbb_sort_firsttime;
	} else if (menuitem == ubertooth->mi_sort_lasttime) {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_lasttime, 1);
		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SORT", "lasttime", 
												  globalreg->timestamp.tv_sec);
		ubertooth->sort_type = btbb_sort_lasttime;
	} else if (menuitem == ubertooth->mi_sort_packets) {
		ubertooth->menu->SetMenuItemChecked(ubertooth->mi_sort_packets, 1);
		ubertooth->pdata->kpinterface->prefs->SetOpt("PLUGIN_UBERTOOTH_SORT", "packets", 
												  globalreg->timestamp.tv_sec);
		ubertooth->sort_type = btbb_sort_packets;
	}
}

void UbertoothProtoBTBBDEV(CLIPROTO_CB_PARMS) {
	ubertooth_data *ubertooth = (ubertooth_data *) auxptr;

	if (proto_parsed->size() < ubertooth->asm_btbbdev_num) {
		_MSG("Invalid BTBBDEV sentence from server", MSGFLAG_INFO);
		return;
	}

	int fnum = 0;

	btbb_network *btn = NULL;

	unsigned int tuint;
	uint32_t lap;
	mac_addr ma;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%u", &tuint) != 1)
		return;
	lap = tuint;

	map<uint32_t, btbb_network *>::iterator bti;
	string tstr;
	float tfloat;
	unsigned long tulong;

	if ((bti = ubertooth->btdev_map.find(lap)) == ubertooth->btdev_map.end()) {
		btn = new btbb_network;
		btn->lap = lap;

		ubertooth->btdev_map[lap] = btn;

		ubertooth->btdev_vec.push_back(btn);
	} else {
		btn = bti->second;
	}

	ma = mac_addr((*proto_parsed)[fnum++].word);
	if (ma.error)
		return;
	btn->bd_addr = ma;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%u", &tuint) != 1) {
		return;
	}
	btn->first_time = tuint;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%u", &tuint) != 1) {
		return;
	}
	btn->last_time = tuint;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%u", &tuint) != 1) {
		return;
	}
	btn->num_packets = tuint;

	// Only apply fixed value if we weren't before, never degrade
	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%u", &tuint) != 1) {
		return;
	}
	if (btn->gpsdata.gps_valid == 0)
		btn->gpsdata.gps_valid = tuint;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.min_lat = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.min_lon = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.min_alt = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.min_spd = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.max_lat = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.max_lon = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.max_alt = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.max_spd = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.aggregate_lat = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.aggregate_lon = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%f", &tfloat) != 1) 
		return;
	btn->gpsdata.aggregate_alt = tfloat;

	if (sscanf((*proto_parsed)[fnum++].word.c_str(), "%lu", &tulong) != 1) 
		return;
	btn->gpsdata.aggregate_points = tulong;
}

void UbertoothCliConfigured(CLICONF_CB_PARMS) {
	ubertooth_data *ubertooth = (ubertooth_data *) auxptr;

	// Wipe the scanlist
	ubertooth->btdevlist->Clear();

	if (kcli->RegisterProtoHandler("BTBBDEV", ubertooth->asm_btbbdev_fields,
								   UbertoothProtoBTBBDEV, auxptr) < 0) {
		_MSG("Could not register BTBBDEV protocol with remote server", MSGFLAG_ERROR);

		globalreg->panel_interface->RaiseAlert("No Bluetooth baseband protocol",
				"The Ubertooth UI was unable to register the required\n"
				"BTBBDEV protocol.  Either it is unavailable\n"
				"(you didn't load the Ubertooth server plugin) or you\n"
				"are using an older server plugin.\n");
		return;
	}
}

void UbertoothCliAdd(KPI_ADDCLI_CB_PARMS) {
	if (add == 0)
		return;

	netcli->AddConfCallback(UbertoothCliConfigured, 1, auxptr);
}

int UbertoothTimer(TIMEEVENT_PARMS) {
#ifndef KIS_NEW_TIMER_PARM
	ubertooth_data *ubertooth = (ubertooth_data *) parm;
#else
	ubertooth_data *ubertooth = (ubertooth_data *) auxptr;
#endif

	// This isn't efficient at all.. but pull the current line, re-sort the 
	// data vector, clear the display, recreate the strings in the table, 
	// re-insert them, and reset the position to the stored one

	vector<string> current_row = ubertooth->btdevlist->GetSelectedData();

	mac_addr current_bdaddr;

	if (current_row.size() >= 1) 
		current_bdaddr = mac_addr(current_row[0]);

	vector<string> add_row;

	switch (ubertooth->sort_type) {
		case btbb_sort_bdaddr:
			stable_sort(ubertooth->btdev_vec.begin(), ubertooth->btdev_vec.end(),
						Ubertooth_Sort_Bdaddr());
			break;
		case btbb_sort_firsttime:
			stable_sort(ubertooth->btdev_vec.begin(), ubertooth->btdev_vec.end(),
						Ubertooth_Sort_Firsttime());
			break;
		case btbb_sort_lasttime:
			stable_sort(ubertooth->btdev_vec.begin(), ubertooth->btdev_vec.end(),
						Ubertooth_Sort_Lasttime());
			break;
		case btbb_sort_packets:
			stable_sort(ubertooth->btdev_vec.begin(), ubertooth->btdev_vec.end(),
						Ubertooth_Sort_Packets());
			break;
		default:
			break;
	}

	ubertooth->btdevlist->Clear();

	for (unsigned int x = 0; x < ubertooth->btdev_vec.size(); x++) {
		add_row.clear();

		add_row.push_back(ubertooth->btdev_vec[x]->bd_addr.Mac2String());
		add_row.push_back(IntToString(ubertooth->btdev_vec[x]->num_packets));

		ubertooth->btdevlist->AddRow(x, add_row);

		if (ubertooth->btdev_vec[x]->bd_addr == current_bdaddr)
			ubertooth->btdevlist->SetSelected(x);
	}

	return 1;
}

int UbertoothDevlistCB(COMPONENT_CALLBACK_PARMS) {
	ubertooth_data *ubertooth = (ubertooth_data *) aux;
	int selected = 0;

	if (ubertooth->btdev_map.size() == 0) {
		globalreg->panel_interface->RaiseAlert("No Ubertooth devices",
			"No Ubertooth devices, can only show details\n"
			"once a device has been found.\n");
		return 1;
	}

	if ((selected = ubertooth->btdevlist->GetSelected()) < 0 ||
			   selected > ubertooth->btdev_vec.size()) {
		globalreg->panel_interface->RaiseAlert("No Ubertooth device selected",
			"No Ubertooth device selected in the Ubertooth list, can\n"
			"only show details once a device has been selected.\n");
		return 1;
	}

	Ubertooth_Details_Panel *dp = 
		new Ubertooth_Details_Panel(globalreg, globalreg->panel_interface);
	dp->SetUbertooth(ubertooth);
	dp->SetDetailsNet(ubertooth->btdev_vec[selected]);
	globalreg->panel_interface->AddPanel(dp);

	return 1;
}

int Ubertooth_Details_ButtonCB(COMPONENT_CALLBACK_PARMS) {
	((Ubertooth_Details_Panel *) aux)->ButtonAction(component);
}

Ubertooth_Details_Panel::Ubertooth_Details_Panel(GlobalRegistry *in_globalreg,
										   KisPanelInterface *in_intf) :
	Kis_Panel(in_globalreg, in_intf) {

	SetTitle("Ubertooth Details");

	btdetailt = new Kis_Free_Text(globalreg, this);
	AddComponentVec(btdetailt, (KIS_PANEL_COMP_DRAW | KIS_PANEL_COMP_EVT |
								KIS_PANEL_COMP_TAB));
	btdetailt->Show();

	closebutton = new Kis_Button(globalreg, this);
	closebutton->SetText("Close");
	closebutton->SetCallback(COMPONENT_CBTYPE_ACTIVATED, Ubertooth_Details_ButtonCB, this);
	AddComponentVec(closebutton, (KIS_PANEL_COMP_DRAW | KIS_PANEL_COMP_EVT |
								  KIS_PANEL_COMP_TAB));
	closebutton->Show();

	vbox = new Kis_Panel_Packbox(globalreg, this);
	vbox->SetPackV();
	vbox->SetHomogenous(0);
	vbox->SetSpacing(0);

	vbox->Pack_End(btdetailt, 1, 0);
	vbox->Pack_End(closebutton, 0, 0);
	AddComponentVec(vbox, KIS_PANEL_COMP_DRAW);

	vbox->Show();

	SetActiveComponent(btdetailt);

	main_component = vbox;

	Position(WIN_CENTER(LINES, COLS));
}

Ubertooth_Details_Panel::~Ubertooth_Details_Panel() {

}

void Ubertooth_Details_Panel::DrawPanel() {
	vector<string> td;

	int selected;

	if (ubertooth == NULL) {
		td.push_back("Ubertooth details panel draw event happened before ubertooth");
		td.push_back("known, something is busted internally, report this");
	} else if (btnet == NULL) {
		td.push_back("No Bluetooth piconet selected");
	} else {
		td.push_back(AlignString("BD_ADDR: ", ' ', 2, 16) + btnet->bd_addr.Mac2String());
		td.push_back(AlignString("Count: ", ' ', 2, 16) + IntToString(btnet->num_packets));
		td.push_back(AlignString("First Seen: ", ' ', 2, 16) +
					string(ctime((const time_t *) 
								 &(btnet->first_time)) + 4).substr(0, 15));
		td.push_back(AlignString("Last Seen: ", ' ', 2, 16) +
					string(ctime((const time_t *) 
								 &(btnet->last_time)) + 4).substr(0, 15));
	}

	btdetailt->SetText(td);

	Kis_Panel::DrawPanel();
}

void Ubertooth_Details_Panel::ButtonAction(Kis_Panel_Component *in_button) {
	if (in_button == closebutton) {
		globalreg->panel_interface->KillPanel(this);
	}
}

void Ubertooth_Details_Panel::MenuAction(int opt) {

}
