#include "DGenApp.h"

#include <Alert.h>
#include <Message.h>
#include <Messenger.h>

#include "DGenWindow.h"
#include "pdHaiku.h"

DGenApp *dgen_app_instance = NULL;

DGenApp::DGenApp()
	: BApplication("application/x-vnd.dgen")
{
	dgen_app_instance = this;
}

void DGenApp::RefsReceived(BMessage *message)
{
	DGenWindow *window = dgen_haiku_window();
	if (window != NULL) {
		BMessenger messenger(window);
		if (messenger.IsValid())
			messenger.SendMessage(message);
	}
}

void DGenApp::AboutRequested()
{
	BAlert *alert = new BAlert("About DGen",
		"DGen " VERSION "\n"
		"Native Haiku front end skeleton.\n"
		"Original DGen for BeOS by Dave.\n"
		"SDL port by Joe Groff and Phil K. Hornung.",
		"OK");
	alert->Go();
}
