#ifndef __DGEN_APP_H__
#define __DGEN_APP_H__

#include <Application.h>

class BMessage;

class DGenApp : public BApplication {
public:
	DGenApp();
	virtual void RefsReceived(BMessage *message);
	virtual void AboutRequested();
};

extern DGenApp *dgen_app_instance;

#endif
