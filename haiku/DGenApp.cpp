#include "DGenApp.h"

#include "DGenLocale.h"

#include <Alert.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>

#include <cstring>

#include "DGenWindow.h"
#include "pdHaiku.h"

DGenApp *dgen_app_instance = NULL;

DGenApp::DGenApp()
	: BApplication("application/x-vnd.dgen")
{
	dgen_app_instance = this;

	BPath settingsDir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsDir, true) == B_OK) {
		fSettingsPath = settingsDir;
		if (fSettingsPath.Append("DGen_settings") == B_OK) {
			BFile file(fSettingsPath.Path(), B_READ_ONLY);
			BMessage settings;
			if (file.InitCheck() == B_OK && settings.Unflatten(&file) == B_OK) {
				BMessage recent;
				if (settings.FindMessage("recent", &recent) == B_OK)
					fRecentFiles = recent;
			}
		}
	}
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
	BAlert *alert = new BAlert(B_TRANSLATE("About DGen"),
		B_TRANSLATE("DGen " VERSION "\n"
			"Native Haiku front end.\n"
			"Original DGen for BeOS by Dave.\n"
			"SDL port by Joe Groff and Phil K. Hornung."),
		B_TRANSLATE("OK"));
	alert->Go();
}

status_t DGenApp::LoadSettings(BMessage &settings) const
{
	if (fSettingsPath.Path() == NULL)
		return B_BAD_VALUE;
	BFile file(fSettingsPath.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	return settings.Unflatten(&file);
}

status_t DGenApp::SaveSettings(const BMessage &settings) const
{
	if (fSettingsPath.Path() == NULL)
		return B_BAD_VALUE;
	BFile file(fSettingsPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	status = settings.Flatten(&file);
	if (status == B_OK)
		file.Sync();
	return status;
}

void DGenApp::AddRecentFile(const char *path)
{
	if (path == NULL || path[0] == '\0')
		return;

	BEntry entry(path);
	if (entry.InitCheck() != B_OK || !entry.Exists())
		return;
	BPath normalized;
	if (entry.GetPath(&normalized) != B_OK || normalized.Path() == NULL)
		return;

	BMessage updated;
	updated.AddString("path", normalized.Path());
	for (int32 i = 0; i < fRecentFiles.CountNames(B_STRING_TYPE) && i < 7; i++) {
		const char *oldPath = fRecentFiles.FindString("path", i);
		if (oldPath == NULL || strcmp(oldPath, normalized.Path()) == 0)
			continue;
		updated.AddString("path", oldPath);
	}
	fRecentFiles = updated;

	BMessage settings;
	LoadSettings(settings);
	settings.RemoveName("recent");
	settings.AddMessage("recent", &fRecentFiles);
	SaveSettings(settings);
}

const BMessage &DGenApp::RecentFiles() const
{
	return fRecentFiles;
}
