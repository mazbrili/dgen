#ifndef __DGEN_WINDOW_H__
#define __DGEN_WINDOW_H__

#include <Window.h>

#include "DGenInput.h"

class BFilePanel;
class BMenu;
class BMenuItem;
class BMenuBar;
class BMessage;
class BStringView;
class BWindow;
class DGenView;

class DGenWindow : public BWindow {
public:
	DGenWindow();
	virtual ~DGenWindow();
	virtual void FrameMoved(BPoint newPosition);
	virtual void FrameResized(float newWidth, float newHeight);
	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();
	DGenView *VideoView() const;

private:
	void ApplyScale(int scale);
	void BuildMenus();
	void LayoutChildren();
	void BuildRecentMenu();
	void SaveSettings() const;
	void ShowOpenPanel();
	void ToggleFullScreen();

	BFilePanel *fOpenPanel;
	BWindow *fGameGenieWindow;
	BWindow *fSettingsWindow;
	BWindow *fInputWindow;
	BMenuBar *fMenuBar;
	BMenu *fRecentMenu;
	DGenView *fView;
	BStringView *fStatus;
	BMenuItem *fFullScreenItem;
	BMenuItem *fStatusItem;
	BMenuItem *fScaleItems[3];
	BRect fNormalFrame;
	bool fPaused;
	bool fFullScreen;
	bool fShowStatus;
	bool fLoadingSettings;
	int fScale;
	DGenInputMap fInputMap;
};

#endif
