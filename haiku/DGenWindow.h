#ifndef __DGEN_WINDOW_H__
#define __DGEN_WINDOW_H__

#include <Window.h>

class BMenuBar;
class BMessage;
class BStringView;
class DGenView;

class DGenWindow : public BWindow {
public:
	DGenWindow();
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();
	DGenView *VideoView() const;

private:
	void BuildMenus();
	void ShowOpenPanel();

	BMenuBar *fMenuBar;
	DGenView *fView;
	BStringView *fStatus;
	bool fPaused;
};

#endif
