#include "DGenWindow.h"

#include <Alert.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <StringView.h>

#include "DGenApp.h"
#include "DGenMessages.h"
#include "DGenView.h"
#include "pdHaiku.h"

DGenWindow::DGenWindow()
	: BWindow(BRect(0, 0, 655, 489), "DGen", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	  fMenuBar(NULL),
	  fView(NULL),
	  fStatus(NULL),
	  fPaused(false)
{
	BuildMenus();
	AddChild(fMenuBar);

	fView = new DGenView(BRect(0, 21, 655, 466));
	AddChild(fView);

	fStatus = new BStringView(BRect(0, 469, 655, 479), "status", "Ready");
	AddChild(fStatus);

	SetSizeLimits(336, 2048, 240, 2048);
	CenterOnScreen();
	dgen_haiku_attach_window(this);
}

void DGenWindow::BuildMenus()
{
	fMenuBar = new BMenuBar(BRect(0, 0, 655, 19), "menu");

	BMenu *menu = new BMenu("DGen");
	menu->AddItem(new BMenuItem("About DGen…", new BMessage(MSG_ABOUT)));
	menu->AddItem(new BMenuItem("Settings…", new BMessage(MSG_SETTINGS), ',',
		B_COMMAND_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(MSG_QUIT), 'Q',
		B_COMMAND_KEY));
	fMenuBar->AddItem(menu);

	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Open ROM…", new BMessage(MSG_OPEN_ROM), 'O',
		B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Open recent", new BMessage(MSG_OPEN_RECENT)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close cartridge",
		new BMessage(MSG_CLOSE_CARTRIDGE), 'W', B_COMMAND_KEY));
	fMenuBar->AddItem(menu);

	menu = new BMenu("Emulation");
	menu->AddItem(new BMenuItem("Pause", new BMessage(MSG_PAUSE), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Reset", new BMessage(MSG_RESET), 'R'));
	menu->AddItem(new BMenuItem("Hard reset", new BMessage(MSG_HARD_RESET)));
	fMenuBar->AddItem(menu);

	menu = new BMenu("View");
	menu->AddItem(new BMenuItem("Full screen",
		new BMessage(MSG_TOGGLE_FULLSCREEN), 'F'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Scale 1×", new BMessage(MSG_SCALE_1X)));
	menu->AddItem(new BMenuItem("Scale 2×", new BMessage(MSG_SCALE_2X)));
	menu->AddItem(new BMenuItem("Scale 3×", new BMessage(MSG_SCALE_3X)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Show status",
		new BMessage(MSG_TOGGLE_STATUS)));
	fMenuBar->AddItem(menu);

	menu = new BMenu("Input");
	menu->AddItem(new BMenuItem("Configure controls…",
		new BMessage(MSG_CONFIGURE_INPUT)));
	menu->AddItem(new BMenuItem("Calibrate joystick…",
		new BMessage(MSG_CALIBRATE)));
	fMenuBar->AddItem(menu);

	menu = new BMenu("Tools");
	menu->AddItem(new BMenuItem("Game Genie…", new BMessage(MSG_GAME_GENIE),
		'G'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Save state…", new BMessage(MSG_SAVE_STATE),
		'S', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Load state…", new BMessage(MSG_LOAD_STATE),
		'L'));
	fMenuBar->AddItem(menu);

	menu = new BMenu("Help");
	menu->AddItem(new BMenuItem("About DGen…", new BMessage(MSG_ABOUT)));
	fMenuBar->AddItem(menu);
}

void DGenWindow::ShowOpenPanel()
{
	BFilePanel *panel = new BFilePanel(B_OPEN_PANEL,
		new BMessenger(this), NULL, 0, false,
		new BMessage(MSG_FILE_SELECTED));
	panel->SetTarget(this);
	panel->Show();
}

void DGenWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case MSG_UPDATE_FRAME:
		if (fView != NULL)
			fView->Invalidate();
		break;
	case MSG_STATUS_TEXT: {
		const char *text = NULL;
		if (message->FindString("text", &text) == B_OK && fStatus != NULL)
			fStatus->SetText(text);
		break;
	}
	case MSG_OPEN_ROM:
	case MSG_OPEN_RECENT:
		ShowOpenPanel();
		break;
	case MSG_FILE_SELECTED: {
		entry_ref ref;
		if (message->FindRef("refs", &ref) == B_OK) {
			BEntry entry(&ref);
			BPath path;
			if (entry.GetPath(&path) == B_OK)
				dgen_haiku_open_rom(path.Path());
		}
		break;
	}
	case MSG_CLOSE_CARTRIDGE:
		if (fStatus != NULL)
			fStatus->SetText("Close cartridge is not wired up yet.");
		break;
	case MSG_PAUSE:
		fPaused = !fPaused;
		dgen_haiku_set_paused(fPaused);
		if (fStatus != NULL)
			fStatus->SetText(fPaused ? "Paused" : "Running");
		break;
	case MSG_RESET:
		dgen_haiku_request_reset(false);
		break;
	case MSG_HARD_RESET:
		dgen_haiku_request_reset(true);
		break;
	case MSG_TOGGLE_FULLSCREEN:
		dgen_haiku_toggle_fullscreen();
		break;
	case MSG_SCALE_1X:
		dgen_haiku_set_scale(1);
		break;
	case MSG_SCALE_2X:
		dgen_haiku_set_scale(2);
		break;
	case MSG_SCALE_3X:
		dgen_haiku_set_scale(3);
		break;
	case MSG_TOGGLE_STATUS:
		if (fStatus != NULL) {
			if (fStatus->IsHidden())
				fStatus->Show();
			else
				fStatus->Hide();
		}
		break;
	case MSG_CONFIGURE_INPUT:
	case MSG_CALIBRATE:
	case MSG_GAME_GENIE:
	case MSG_SAVE_STATE:
	case MSG_LOAD_STATE:
	case MSG_SETTINGS:
		if (fStatus != NULL)
			fStatus->SetText("This dialog is not wired up yet.");
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
	dgen_haiku_request_quit();
	return true;
}

DGenView *DGenWindow::VideoView() const
{
	return fView;
}
