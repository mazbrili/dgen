#ifndef __DGEN_APP_H__
#define __DGEN_APP_H__

#include <Application.h>
#include <Message.h>
#include <Path.h>

class DGenApp : public BApplication {
public:
	DGenApp();
	virtual void RefsReceived(BMessage *message);
	virtual void AboutRequested();

	status_t LoadSettings(BMessage &settings) const;
	status_t SaveSettings(const BMessage &settings) const;
	void AddRecentFile(const char *path);
	const BMessage &RecentFiles() const;

private:
	BPath fSettingsPath;
	BMessage fRecentFiles;
};

extern DGenApp *dgen_app_instance;

#endif
