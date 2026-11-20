#include "DGenWindow.h"

#include "DGenLocale.h"

#include <Alert.h>
#include <Button.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Screen.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include <cstdio>

#include "DGenApp.h"
#include "DGenMessages.h"
#include "DGenView.h"
#include "pdHaiku.h"

class GameGenieWindow : public BWindow {
public:
	GameGenieWindow(BMessenger owner)
		: BWindow(BRect(0, 0, 379, 112), "Game Genie", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		  fOwner(owner)
	{
		BView *view = new BView(Bounds(), "genieView", B_FOLLOW_ALL,
			B_WILL_DRAW);
		AddChild(view);

		fCode = new BTextControl(BRect(12, 12, 367, 38), "code",
			B_TRANSLATE("Code:"), "", NULL);
		fCode->SetDivider(50);
		view->AddChild(fCode);

		BButton *apply = new BButton(BRect(237, 72, 367, 100), "apply",
			B_TRANSLATE("Apply"), new BMessage(MSG_GAME_GENIE_APPLY));
		apply->SetTarget(this);
		apply->MakeDefault(true);
		view->AddChild(apply);

		BButton *cancel = new BButton(BRect(99, 72, 229, 100), "cancel",
			B_TRANSLATE("Cancel"), new BMessage('cncl'));
		cancel->SetTarget(this);
		view->AddChild(cancel);

		CenterOnScreen();
	}

	virtual void MessageReceived(BMessage *message)
	{
		if (message->what == MSG_GAME_GENIE_APPLY) {
			BMessage apply(MSG_GAME_GENIE_APPLY);
			apply.AddString("code", fCode->Text());
			fOwner.SendMessage(&apply);
			Quit();
			return;
		}
		if (message->what == 'cncl') {
			Quit();
			return;
		}
		BWindow::MessageReceived(message);
	}

	virtual bool QuitRequested()
	{
		BMessage closed(MSG_GENIE_CLOSED);
		fOwner.SendMessage(&closed);
		return BWindow::QuitRequested();
	}

private:
	BMessenger fOwner;
	BTextControl *fCode;
};

DGenWindow::DGenWindow()
	: BWindow(BRect(0, 0, 655, 489), "DGen", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	  fOpenPanel(NULL),
	  fGameGenieWindow(NULL),
	  fMenuBar(NULL),
	  fRecentMenu(NULL),
	  fView(NULL),
	  fStatus(NULL),
	  fFullScreenItem(NULL),
	  fStatusItem(NULL),
	  fScaleItems{NULL, NULL, NULL},
	  fPaused(false),
	  fFullScreen(false),
	  fShowStatus(true),
	  fLoadingSettings(true),
	  fScale(2)
{
	bool restoredFrame = false;
	BMessage settings;
	if (dgen_app_instance != NULL &&
		dgen_app_instance->LoadSettings(settings) == B_OK) {
		BRect frame;
		if (settings.FindRect("frame", &frame) == B_OK) {
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(), frame.Height());
			restoredFrame = true;
		}
		if (settings.HasInt32("scale"))
			fScale = settings.FindInt32("scale");
		if (settings.HasBool("show_status"))
			fShowStatus = settings.FindBool("show_status");
	}

	fNormalFrame = Frame();
	BuildMenus();
	AddChild(fMenuBar);

	fView = new DGenView(BRect(0, 21, 655, 466));
	AddChild(fView);

	fStatus = new BStringView(BRect(0, 469, 655, 479), "status",
		B_TRANSLATE("Ready"));
	AddChild(fStatus);

	SetSizeLimits(336, 2048, 240, 2048);
	if (!restoredFrame)
		CenterOnScreen();
	ApplyScale(fScale);
	fLoadingSettings = false;
	dgen_haiku_attach_window(this);
}

DGenWindow::~DGenWindow()
{
	delete fOpenPanel;
	if (fGameGenieWindow != NULL)
		fGameGenieWindow->PostMessage(B_QUIT_REQUESTED);
}

void DGenWindow::FrameMoved(BPoint)
{
	if (fLoadingSettings || fFullScreen)
		return;
	fNormalFrame = Frame();
	SaveSettings();
}

void DGenWindow::FrameResized(float, float)
{
	if (fLoadingSettings || fFullScreen)
		return;
	fNormalFrame = Frame();
	SaveSettings();
}

static void add_state_items(BMenu *menu, uint32 what)
{
	char label[16];
	for (int i = 0; i < 10; i++) {
		BMessage *message = new BMessage(what);
		message->AddInt32("slot", i);
		snprintf(label, sizeof(label), B_TRANSLATE("Slot %d"), i);
		menu->AddItem(new BMenuItem(label, message));
	}
}

void DGenWindow::BuildMenus()
{
	fMenuBar = new BMenuBar(BRect(0, 0, 655, 19), "menu");

	BMenu *menu = new BMenu("DGen");
	menu->AddItem(new BMenuItem(B_TRANSLATE("About DGen…"), new BMessage(MSG_ABOUT)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings…"), new BMessage(MSG_SETTINGS), ',',
		B_COMMAND_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(MSG_QUIT), 'Q',
		B_COMMAND_KEY));
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open ROM…"), new BMessage(MSG_OPEN_ROM), 'O',
		B_COMMAND_KEY));
	fRecentMenu = new BMenu(B_TRANSLATE("Open recent"));
	menu->AddItem(fRecentMenu);
	BuildRecentMenu();
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close cartridge"),
		new BMessage(MSG_CLOSE_CARTRIDGE), 'W', B_COMMAND_KEY));
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Emulation"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Pause"), new BMessage(MSG_PAUSE), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Reset"), new BMessage(MSG_RESET), 'R'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Hard reset"), new BMessage(MSG_HARD_RESET)));
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("View"));
	fFullScreenItem = new BMenuItem(B_TRANSLATE("Full screen"),
		new BMessage(MSG_TOGGLE_FULLSCREEN), 'F');
	menu->AddItem(fFullScreenItem);
	menu->AddSeparatorItem();
	for (int i = 0; i < 3; i++) {
		BMessage *message = new BMessage(MSG_SCALE_1X + i);
		fScaleItems[i] = new BMenuItem(B_TRANSLATE("Scale 1×"), message);
		menu->AddItem(fScaleItems[i]);
	}
	menu->AddSeparatorItem();
	fStatusItem = new BMenuItem(B_TRANSLATE("Show status"),
		new BMessage(MSG_TOGGLE_STATUS));
	menu->AddItem(fStatusItem);
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Input"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Configure controls…"),
		new BMessage(MSG_CONFIGURE_INPUT)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Calibrate joystick…"),
		new BMessage(MSG_CALIBRATE)));
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Tools"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Game Genie…"), new BMessage(MSG_GAME_GENIE),
		'G'));
	menu->AddSeparatorItem();
	BMenu *saveMenu = new BMenu(B_TRANSLATE("Save state"));
	add_state_items(saveMenu, MSG_SAVE_STATE);
	menu->AddItem(saveMenu);
	BMenu *loadMenu = new BMenu(B_TRANSLATE("Load state"));
	add_state_items(loadMenu, MSG_LOAD_STATE);
	menu->AddItem(loadMenu);
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Help"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("About DGen…"), new BMessage(MSG_ABOUT)));
	fMenuBar->AddItem(menu);
}

void DGenWindow::BuildRecentMenu()
{
	if (fRecentMenu == NULL)
		return;

	BMenuItem *item = NULL;
	while ((item = fRecentMenu->ItemAt(0)) != NULL) {
		fRecentMenu->RemoveItem(item);
		delete item;
	}

	const BMessage &recent = dgen_app_instance != NULL
		? dgen_app_instance->RecentFiles() : BMessage();
	int32 count = recent.CountNames(B_STRING_TYPE);
	if (count == 0) {
		item = new BMenuItem(B_TRANSLATE("No recent files"), NULL);
		item->SetEnabled(false);
		fRecentMenu->AddItem(item);
		return;
	}

	for (int32 i = 0; i < count && i < 8; i++) {
		const char *path = recent.FindString("path", i);
		if (path == NULL)
			continue;
		BMessage *message = new BMessage(MSG_OPEN_RECENT);
		message->AddString("path", path);
		fRecentMenu->AddItem(new BMenuItem(path, message));
	}
}

void DGenWindow::LayoutChildren()
{
	BRect frame = Frame();
	float width = frame.Width();
	float height = frame.Height();

	if (fFullScreen) {
		if (fMenuBar != NULL)
			fMenuBar->Hide();
		if (fStatus != NULL)
			fStatus->Hide();
		if (fView != NULL) {
			fView->MoveTo(0, 0);
			fView->ResizeTo(width, height);
		}
		return;
	}

	if (fMenuBar != NULL) {
		fMenuBar->MoveTo(0, 0);
		fMenuBar->ResizeTo(width, 19);
		fMenuBar->Show();
	}
	if (fView != NULL) {
		fView->MoveTo(0, 21);
		fView->ResizeTo(width, height - 23);
	}
	if (fStatus != NULL) {
		fStatus->MoveTo(0, height - 20);
		fStatus->ResizeTo(width, 11);
		if (fShowStatus)
			fStatus->Show();
		else
			fStatus->Hide();
	}
}

void DGenWindow::ApplyScale(int scale)
{
	if (scale < 1)
		scale = 1;
	if (scale > 3)
		scale = 3;
	fScale = scale;

	if (!fFullScreen) {
		ResizeTo(320 * scale + 15, 224 * scale + 41);
		fNormalFrame = Frame();
	}
	LayoutChildren();

	for (int i = 0; i < 3; i++) {
		if (fScaleItems[i] != NULL) {
			fScaleItems[i]->SetMarked(i + 1 == fScale);
			switch (i) {
			case 0:
				fScaleItems[i]->SetLabel(B_TRANSLATE("Scale 1×"));
				break;
			case 1:
				fScaleItems[i]->SetLabel(B_TRANSLATE("Scale 2×"));
				break;
			case 2:
				fScaleItems[i]->SetLabel(B_TRANSLATE("Scale 3×"));
				break;
			}
		}
	}
	if (fFullScreenItem != NULL)
		fFullScreenItem->SetMarked(fFullScreen);
	if (fStatusItem != NULL)
		fStatusItem->SetMarked(fShowStatus);

	SaveSettings();
}

void DGenWindow::ToggleFullScreen()
{
	BScreen screen;
	if (!screen.IsValid())
		return;

	if (fFullScreen) {
		fFullScreen = false;
		MoveTo(fNormalFrame.LeftTop());
		ResizeTo(fNormalFrame.Width(), fNormalFrame.Height());
	} else {
		fNormalFrame = Frame();
		fFullScreen = true;
		BRect frame = screen.Frame();
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
	LayoutChildren();
	if (fFullScreenItem != NULL)
		fFullScreenItem->SetMarked(fFullScreen);
	SaveSettings();
}

void DGenWindow::SaveSettings() const
{
	BMessage settings;
	if (dgen_app_instance == NULL)
		return;
	dgen_app_instance->LoadSettings(settings);

	settings.RemoveName("frame");
	settings.RemoveName("scale");
	settings.RemoveName("show_status");
	settings.AddRect("frame", fFullScreen ? fNormalFrame : Frame());
	settings.AddInt32("scale", fScale);
	settings.AddBool("show_status", fShowStatus);
	dgen_app_instance->SaveSettings(settings);
}

void DGenWindow::ShowOpenPanel()
{
	if (fOpenPanel == NULL) {
		BMessenger target(this);
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, &target, NULL, 0, false,
			new BMessage(MSG_FILE_SELECTED));
	}
	fOpenPanel->Show();
}

static void open_first_ref(BMessage *message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return;
	BEntry entry(&ref);
	BPath path;
	if (entry.GetPath(&path) == B_OK)
		dgen_haiku_open_rom(path.Path());
}

void DGenWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case MSG_UPDATE_FRAME:
		if (fView != NULL)
			fView->Invalidate();
		break;
	case MSG_STATUS_TEXT: {
		const char *text = message->FindString("text");
		if (text != NULL && fStatus != NULL)
			fStatus->SetText(text);
		break;
	}
	case MSG_OPEN_ROM:
		ShowOpenPanel();
		break;
	case MSG_OPEN_RECENT: {
		const char *path = message->FindString("path");
		if (path != NULL)
			dgen_haiku_open_rom(path);
		else
			ShowOpenPanel();
		break;
	}
	case MSG_REBUILD_RECENT:
		BuildRecentMenu();
		break;
	case MSG_FILE_SELECTED:
		if (fOpenPanel != NULL)
			fOpenPanel->Hide();
		open_first_ref(message);
		break;
	case B_REFS_RECEIVED:
		open_first_ref(message);
		break;
	case MSG_CLOSE_CARTRIDGE:
		dgen_haiku_request_close_cartridge();
		break;
	case MSG_PAUSE:
		fPaused = !fPaused;
		dgen_haiku_set_paused(fPaused);
		if (fStatus != NULL)
			fStatus->SetText(fPaused ? B_TRANSLATE("Paused") :
				B_TRANSLATE("Running"));
		break;
	case MSG_RESET:
		dgen_haiku_request_reset(false);
		break;
	case MSG_HARD_RESET:
		dgen_haiku_request_reset(true);
		break;
	case MSG_TOGGLE_FULLSCREEN:
		ToggleFullScreen();
		break;
	case MSG_SET_SCALE:
		ApplyScale(message->FindInt32("scale"));
		break;
	case MSG_SCALE_1X:
	case MSG_SCALE_2X:
	case MSG_SCALE_3X:
		ApplyScale(message->what - MSG_SCALE_1X + 1);
		break;
	case MSG_TOGGLE_STATUS:
		fShowStatus = !fShowStatus;
		LayoutChildren();
		SaveSettings();
		break;
	case MSG_CONFIGURE_INPUT:
	case MSG_CALIBRATE:
	case MSG_SETTINGS:
		if (fStatus != NULL)
			fStatus->SetText(B_TRANSLATE("This dialog is not wired up yet."));
		break;
	case MSG_GAME_GENIE:
		if (fGameGenieWindow == NULL)
			fGameGenieWindow = new GameGenieWindow(BMessenger(this));
		else
			fGameGenieWindow->Activate();
		break;
	case MSG_GAME_GENIE_APPLY: {
		const char *code = message->FindString("code");
		if (code != NULL)
			dgen_haiku_request_game_genie(code);
		break;
	}
	case MSG_GENIE_CLOSED:
		fGameGenieWindow = NULL;
		break;
	case MSG_SAVE_STATE:
		dgen_haiku_request_save_state(message->FindInt32("slot"));
		break;
	case MSG_LOAD_STATE:
		dgen_haiku_request_load_state(message->FindInt32("slot"));
		break;
	case MSG_ABOUT:
		if (dgen_app_instance != NULL)
			dgen_app_instance->AboutRequested();
		break;
	case MSG_QUIT:
		dgen_haiku_request_quit();
		Quit();
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool DGenWindow::QuitRequested()
{
	SaveSettings();
	dgen_haiku_request_quit();
	return true;
}

DGenView *DGenWindow::VideoView() const
{
	return fView;
}
