#include "DGenWindow.h"

#include "DGenLocale.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <RadioButton.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include <cstdio>

#include "DGenApp.h"
#ifdef WITH_JOYSTICK
#include "DGenJoystick.h"
#endif
#include "DGenMessages.h"
#include "DGenView.h"
#include "pdHaiku.h"

static bool message_bool(const BMessage &message, const char *name,
	bool defaultValue = false)
{
	if (message.HasBool(name))
		return message.FindBool(name);
	return defaultValue;
}

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
		Show();
	}

	virtual void MessageReceived(BMessage *message)
	{
		if (message->what == MSG_GAME_GENIE_APPLY) {
			BMessage apply(MSG_GAME_GENIE_APPLY);
			apply.AddString("code", fCode->Text());
			fOwner.SendMessage(&apply);
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		if (message->what == 'cncl') {
			PostMessage(B_QUIT_REQUESTED);
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

class SettingsWindow : public BWindow {
public:
	SettingsWindow(BMessenger owner, bool sound, bool pal)
		: BWindow(BRect(0, 0, 304, 156), B_TRANSLATE("DGen settings"),
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		  fOwner(owner)
	{
		BView *view = new BView(Bounds(), "settingsView", B_FOLLOW_ALL,
			B_WILL_DRAW);
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(view);

		fSound = new BCheckBox(BRect(12, 12, 292, 34), "sound",
			B_TRANSLATE("Enable sound"), new BMessage('stts'));
		fSound->SetValue(sound ? B_CONTROL_ON : B_CONTROL_OFF);
		fSound->SetTarget(this);
		view->AddChild(fSound);

		fNtsc = new BRadioButton(BRect(28, 46, 140, 68), "ntsc",
			B_TRANSLATE("NTSC, 60 Hz"), new BMessage('stts'));
		fNtsc->SetValue(pal ? B_CONTROL_OFF : B_CONTROL_ON);
		fNtsc->SetTarget(this);
		view->AddChild(fNtsc);

		fPal = new BRadioButton(BRect(160, 46, 292, 68), "pal",
			B_TRANSLATE("PAL, 50 Hz"), new BMessage('stts'));
		fPal->SetValue(pal ? B_CONTROL_ON : B_CONTROL_OFF);
		fPal->SetTarget(this);
		view->AddChild(fPal);

		BMessage *applyMessage = new BMessage(MSG_SETTINGS_APPLY);
		applyMessage->AddBool("close", false);
		BButton *apply = new BButton(BRect(175, 116, 292, 144), "apply",
			B_TRANSLATE("Apply"), applyMessage);
		apply->SetTarget(this);
		apply->MakeDefault(true);
		view->AddChild(apply);

		BMessage *okMessage = new BMessage(MSG_SETTINGS_APPLY);
		okMessage->AddBool("close", true);
		BButton *ok = new BButton(BRect(108, 116, 168, 144), "ok",
			B_TRANSLATE("OK"), okMessage);
		ok->SetTarget(this);
		view->AddChild(ok);

		BButton *cancel = new BButton(BRect(16, 116, 100, 144), "cancel",
			B_TRANSLATE("Cancel"), new BMessage('cncl'));
		cancel->SetTarget(this);
		view->AddChild(cancel);

		CenterOnScreen();
		Show();
	}

	virtual void MessageReceived(BMessage *message)
	{
		if (message->what == MSG_SETTINGS_APPLY) {
			BMessage apply(MSG_SETTINGS_APPLY);
			apply.AddBool("sound", fSound->Value() == B_CONTROL_ON);
			apply.AddBool("pal", fPal->Value() == B_CONTROL_ON);
			bool close = false;
			close = message_bool(*message, "close");
			apply.AddBool("close", close);
			fOwner.SendMessage(&apply);
			return;
		}
		if (message->what == 'cncl') {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		BWindow::MessageReceived(message);
	}

	virtual bool QuitRequested()
	{
		BMessage closed(MSG_SETTINGS_CLOSED);
		fOwner.SendMessage(&closed);
		return BWindow::QuitRequested();
	}

private:
	BMessenger fOwner;
	BCheckBox *fSound;
	BRadioButton *fNtsc;
	BRadioButton *fPal;
};

class DGenKeyButton : public BButton {
public:
	DGenKeyButton(BRect frame, const char *name, const char *label,
		int player, int action)
		: BButton(frame, name, label, NULL),
		  fPlayer(player),
		  fAction(action)
	{
	}

	virtual void KeyDown(BMessage *message)
	{
		DGenKey key;
		if (!dgen_input_key_from_message(*message, key))
			return;

		BMessage captured(MSG_INPUT_CAPTURE);
		captured.AddInt32("player", fPlayer);
		captured.AddInt32("action", fAction);
		captured.AddInt32("modifiers", (int32)key.modifiers);
		captured.AddInt32("key", key.key);
		captured.AddInt32("byteValue", key.byteValue);
		if (Window() != NULL)
			Window()->PostMessage(&captured);
	}

private:
	int fPlayer;
	int fAction;
};

class ConfigureInputWindow : public BWindow {
public:
	ConfigureInputWindow(BMessenger owner, const DGenInputMap &map)
		: BWindow(BRect(0, 0, 720, 420), B_TRANSLATE("Configure controls"),
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		  fOwner(owner),
		  fMap(map),
		  fHint(NULL),
		  fCapturePlayer(-1),
		  fCaptureAction(-1),
		  fSelectedPlayer(-1),
		  fSelectedAction(-1)
	{
		BView *view = new BView(Bounds(), "inputView", B_FOLLOW_ALL,
			B_WILL_DRAW);
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(view);

		for (int player = 0; player < 2; player++) {
			float x = 12 + player * 350;
			BString heading;
			heading.SetToFormat(B_TRANSLATE("Player %d"), player + 1);
			BStringView *title = new BStringView(BRect(x, 10, x + 338, 28),
				"player", heading.String());
			title->SetFont(be_bold_font);
			view->AddChild(title);

			BStringView *keyHeader = new BStringView(
				BRect(x + 82, 30, x + 190, 46), "keyHeader",
				B_TRANSLATE("Keyboard"));
			view->AddChild(keyHeader);
#ifdef WITH_JOYSTICK
			BStringView *joyHeader = new BStringView(
				BRect(x + 194, 30, x + 338, 46), "joyHeader",
				B_TRANSLATE("Joystick"));
			view->AddChild(joyHeader);
#endif

			for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
				float y = 52 + action * 24;
				BStringView *label = new BStringView(
					BRect(x, y, x + 78, y + 18), "action",
					B_TRANSLATE(dgen_input_action_label(action)));
				view->AddChild(label);

				BString keyLabel = dgen_input_key_label(
					fMap.keys[player][action]);
				DGenKeyButton *button = new DGenKeyButton(
					BRect(x + 82, y - 2, x + 190, y + 20), "key",
					keyLabel.String(), player, action);
				fButtons[player][action] = button;
				view->AddChild(button);

#ifdef WITH_JOYSTICK
				BMessage *joyMessage = new BMessage(
					MSG_JOYSTICK_CAPTURE_BEGIN);
				joyMessage->AddInt32("player", player);
				joyMessage->AddInt32("action", action);
				BString joyLabel("J: ");
				joyLabel += dgen_input_joystick_label(
					fMap.joystick[player][action]);
				BButton *joyButton = new BButton(
					BRect(x + 194, y - 2, x + 338, y + 20), "joystick",
					joyLabel.String(), joyMessage);
				joyButton->SetTarget(this);
				fJoyButtons[player][action] = joyButton;
				view->AddChild(joyButton);
#endif
			}
		}

		fHint = new BStringView(BRect(12, 344, 708, 364), "hint",
			B_TRANSLATE("Click a button, then press a key or move a joystick."));
		view->AddChild(fHint);

		BButton *reset = new BButton(BRect(12, 374, 132, 404), "reset",
			B_TRANSLATE("Reset defaults"), new BMessage(MSG_INPUT_RESET));
		reset->SetTarget(this);
		view->AddChild(reset);

		BButton *clear = new BButton(BRect(140, 374, 338, 404), "clearjoy",
			B_TRANSLATE("Clear joystick"), new BMessage(MSG_JOYSTICK_CLEAR));
		clear->SetTarget(this);
#ifndef WITH_JOYSTICK
		clear->SetEnabled(false);
#endif
		view->AddChild(clear);

		BButton *cancel = new BButton(BRect(340, 374, 448, 404), "cancel",
			B_TRANSLATE("Cancel"), new BMessage('cncl'));
		cancel->SetTarget(this);
		view->AddChild(cancel);

		BMessage *okMessage = new BMessage(MSG_INPUT_APPLY);
		okMessage->AddBool("close", true);
		BButton *ok = new BButton(BRect(456, 374, 540, 404), "ok",
			B_TRANSLATE("OK"), okMessage);
		ok->SetTarget(this);
		view->AddChild(ok);

		BMessage *applyMessage = new BMessage(MSG_INPUT_APPLY);
		applyMessage->AddBool("close", false);
		BButton *apply = new BButton(BRect(548, 374, 708, 404), "apply",
			B_TRANSLATE("Apply"), applyMessage);
		apply->SetTarget(this);
		apply->MakeDefault(true);
		view->AddChild(apply);

		CenterOnScreen();
		Show();
	}

	virtual void MessageReceived(BMessage *message)
	{
		switch (message->what) {
		case MSG_INPUT_CAPTURE: {
			int player = message->FindInt32("player");
			int action = message->FindInt32("action");
			DGenKey key;
			if (dgen_input_key_from_message(*message, key) &&
				player >= 0 && player < 2 && action >= 0 &&
				action < DGEN_INPUT_ACTION_COUNT) {
				fMap.keys[player][action] = key;
				dgen_input_clear_conflicts(fMap, player, action);
				BString label = dgen_input_key_label(key);
				fButtons[player][action]->SetLabel(label.String());
			}
			break;
		}
#ifdef WITH_JOYSTICK
		case MSG_JOYSTICK_CAPTURE_BEGIN: {
			int player = message->FindInt32("player");
			int action = message->FindInt32("action");
			if (player >= 0 && player < 2 && action >= 0 &&
				action < DGEN_INPUT_ACTION_COUNT) {
				EndJoystickCapture();
				fCapturePlayer = player;
				fCaptureAction = action;
				fSelectedPlayer = player;
				fSelectedAction = action;
				fJoyButtons[player][action]->SetLabel(
					B_TRANSLATE("Waiting..."));
				fHint->SetText(B_TRANSLATE(
					"Press a joystick button, move an axis, or tilt a hat."));
				dgen_haiku_joystick_begin_capture(BMessenger(this), player);
			}
			break;
		}
		case MSG_JOYSTICK_CAPTURE: {
			int player = message->FindInt32("player");
			int action = fCaptureAction;
			int32 code = message->FindInt32("code");
			if (fCapturePlayer == player && code != 0 && player >= 0 &&
				player < 2 && action >= 0 &&
				action < DGEN_INPUT_ACTION_COUNT) {
				fMap.joystick[player][action] = (uint32)code;
				dgen_input_clear_joystick_conflicts(fMap, player, action,
					(uint32)code);
				EndJoystickCapture();
				RefreshJoyButtons();
				fHint->SetText(B_TRANSLATE(
					"Click a button, then press a key or move a joystick."));
			}
			break;
		}
		case MSG_JOYSTICK_CLEAR: {
			EndJoystickCapture();
			if (fSelectedPlayer >= 0 && fSelectedAction >= 0) {
				fMap.joystick[fSelectedPlayer][fSelectedAction] = 0;
				RefreshJoyButtons();
				fHint->SetText(B_TRANSLATE(
					"Click a button, then press a key or move a joystick."));
			} else {
				fHint->SetText(B_TRANSLATE(
					"Select a joystick binding to clear."));
			}
			break;
		}
#endif
		case MSG_INPUT_RESET:
			EndJoystickCapture();
			dgen_input_defaults(fMap);
			fSelectedPlayer = -1;
			fSelectedAction = -1;
			RefreshButtons();
			RefreshJoyButtons();
			fHint->SetText(B_TRANSLATE(
				"Click a button, then press a key or move a joystick."));
			break;
		case MSG_INPUT_APPLY: {
			EndJoystickCapture();
			BMessage apply(MSG_INPUT_APPLY);
			dgen_input_save(apply, fMap);
			bool close = false;
			close = message_bool(*message, "close");
			apply.AddBool("close", close);
			fOwner.SendMessage(&apply);
			break;
		}
		case 'cncl':
			EndJoystickCapture();
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
		}
	}

	virtual bool QuitRequested()
	{
		EndJoystickCapture();
		BMessage closed(MSG_INPUT_CLOSED);
		fOwner.SendMessage(&closed);
		return BWindow::QuitRequested();
	}

private:
	void EndJoystickCapture()
	{
#ifdef WITH_JOYSTICK
		dgen_haiku_joystick_end_capture();
#endif
		fCapturePlayer = -1;
		fCaptureAction = -1;
	}

	void RefreshButtons()
	{
		for (int player = 0; player < 2; player++) {
			for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
				BString label = dgen_input_key_label(fMap.keys[player][action]);
				fButtons[player][action]->SetLabel(label.String());
			}
		}
	}

	void RefreshJoyButtons()
	{
#ifdef WITH_JOYSTICK
		for (int player = 0; player < 2; player++) {
			for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
				BString label("J: ");
				label += dgen_input_joystick_label(
					fMap.joystick[player][action]);
				fJoyButtons[player][action]->SetLabel(label.String());
			}
		}
#endif
	}

	BMessenger fOwner;
	DGenInputMap fMap;
	DGenKeyButton *fButtons[2][DGEN_INPUT_ACTION_COUNT];
#ifdef WITH_JOYSTICK
	BButton *fJoyButtons[2][DGEN_INPUT_ACTION_COUNT];
#endif
	BStringView *fHint;
	int fCapturePlayer;
	int fCaptureAction;
	int fSelectedPlayer;
	int fSelectedAction;
};

static void open_joystick_preferences()
{
	BEntry entry("/boot/system/preferences/Joystick");
	entry_ref ref;
	if (entry.InitCheck() == B_OK && entry.GetRef(&ref) == B_OK &&
		be_roster->Launch(&ref) == B_OK)
		return;

	BAlert *alert = new BAlert(B_TRANSLATE("Joystick calibration"),
		B_TRANSLATE("No joystick calibration tool is available. "
			"Connect and calibrate a joystick with the Haiku Joystick "
			"preferences, then use Input → Configure controls to change "
			"keyboard or joystick bindings."),
		B_TRANSLATE("OK"));
	alert->Go();
}

static void show_help()
{
	BAlert *alert = new BAlert(B_TRANSLATE("DGen Help"),
		B_TRANSLATE("Open a ROM with File → Open ROM…, then use the keyboard "
			"controls configured in Input → Configure controls… or a "
			"calibrated joystick.\n\n"
			"Escape quits DGen. Use Emulation → Pause to pause, and Tools → "
			"Save state or Load state to keep your place."),
		B_TRANSLATE("OK"));
	alert->Go();
}

DGenWindow::DGenWindow()
	: BWindow(BRect(0, 0, 655, 489), "DGen", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	  fOpenPanel(NULL),
	  fGameGenieWindow(NULL),
	  fSettingsWindow(NULL),
	  fInputWindow(NULL),
	  fMenuBar(NULL),
	  fRecentMenu(NULL),
	  fView(NULL),
	  fStatus(NULL),
	  fFullScreenItem(NULL),
	  fPauseItem(NULL),
	  fStatusItem(NULL),
	  fScaleItems{NULL, NULL, NULL},
	  fPaused(false),
	  fFullScreen(false),
	  fShowStatus(true),
	  fLoadingSettings(true),
	  fScale(2)
{
	bool restoredFrame = false;
	bool sound = true;
	bool pal = false;
	bool hasEmulationSettings = false;
	dgen_input_defaults(fInputMap);
	dgen_input_load_current_joystick(fInputMap);
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
		if (settings.HasBool("sound")) {
			sound = settings.FindBool("sound");
			hasEmulationSettings = true;
		}
		if (settings.HasBool("pal")) {
			pal = settings.FindBool("pal");
			hasEmulationSettings = true;
		}
		dgen_input_load(settings, fInputMap);
	}
	dgen_input_apply_joystick(fInputMap);
	if (hasEmulationSettings)
		dgen_haiku_set_emulation_settings(sound, pal, pal ? 50 : 60);

	fNormalFrame = Frame();
	BuildMenus();
	AddChild(fMenuBar);

	fView = new DGenView(BRect(0, 21, 655, 466), &fInputMap);
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
	dgen_haiku_attach_window(NULL);
	delete fOpenPanel;
	if (fGameGenieWindow != NULL)
		fGameGenieWindow->PostMessage(B_QUIT_REQUESTED);
	if (fSettingsWindow != NULL)
		fSettingsWindow->PostMessage(B_QUIT_REQUESTED);
	if (fInputWindow != NULL)
		fInputWindow->PostMessage(B_QUIT_REQUESTED);
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

void DGenWindow::WindowActivated(bool active)
{
	BWindow::WindowActivated(active);
	if (!active || fView == NULL)
		return;
	if ((fGameGenieWindow != NULL && fGameGenieWindow->IsActive()) ||
		(fSettingsWindow != NULL && fSettingsWindow->IsActive()) ||
		(fInputWindow != NULL && fInputWindow->IsActive()))
		return;
	fView->MakeFocus();
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
	fPauseItem = new BMenuItem(B_TRANSLATE("Pause"), new BMessage(MSG_PAUSE), 'P');
	menu->AddItem(fPauseItem);
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
		BString label;
		label.SetToFormat(B_TRANSLATE("Scale %d×"), i + 1);
		fScaleItems[i] = new BMenuItem(label.String(), message);
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
	menu->AddItem(new BMenuItem(B_TRANSLATE("DGen Help…"), new BMessage(MSG_HELP)));
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
	settings.RemoveName("sound");
	settings.RemoveName("pal");
	settings.AddRect("frame", fFullScreen ? fNormalFrame : Frame());
	settings.AddInt32("scale", fScale);
	settings.AddBool("show_status", fShowStatus);

	bool sound = true;
	bool pal = false;
	int hz = 60;
	dgen_haiku_get_emulation_settings(sound, pal, hz);
	settings.AddBool("sound", sound);
	settings.AddBool("pal", pal);
	dgen_input_save(settings, fInputMap);
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
	case MSG_SET_FRAME_BITMAP: {
		BBitmap *bitmap = NULL;
		BBitmap *oldBitmap = NULL;
		message->FindPointer("bitmap", (void **)&bitmap);
		message->FindPointer("old", (void **)&oldBitmap);
		if (fView != NULL && bitmap != NULL)
			fView->SetFrameBitmap(bitmap);
		delete oldBitmap;
		break;
	}
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
		if (fView != NULL)
			fView->MakeFocus();
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
		if (fPauseItem != NULL)
			fPauseItem->SetMarked(fPaused);
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
		if (fInputWindow == NULL)
			fInputWindow = new ConfigureInputWindow(BMessenger(this), fInputMap);
		else
			fInputWindow->Activate();
		break;
	case MSG_CALIBRATE:
		open_joystick_preferences();
		break;
	case MSG_SETTINGS: {
		bool sound = true;
		bool pal = false;
		int hz = 60;
		dgen_haiku_get_emulation_settings(sound, pal, hz);
		if (fSettingsWindow == NULL)
			fSettingsWindow = new SettingsWindow(BMessenger(this), sound, pal);
		else
			fSettingsWindow->Activate();
		break;
	}
	case MSG_SETTINGS_APPLY: {
		bool sound = false;
		bool pal = false;
		bool close = false;
		sound = message_bool(*message, "sound", sound);
		pal = message_bool(*message, "pal", pal);
		close = message_bool(*message, "close");
		dgen_haiku_set_emulation_settings(sound, pal, pal ? 50 : 60);
		SaveSettings();
		if (close && fSettingsWindow != NULL) {
			BWindow *window = fSettingsWindow;
			fSettingsWindow = NULL;
			window->PostMessage(B_QUIT_REQUESTED);
		}
		break;
	}
	case MSG_SETTINGS_CLOSED:
		fSettingsWindow = NULL;
		break;
	case MSG_INPUT_APPLY: {
		bool close = false;
		close = message_bool(*message, "close");
		dgen_input_load(*message, fInputMap);
		dgen_input_apply_joystick(fInputMap);
		if (fView != NULL)
			fView->SetInputMap(&fInputMap);
		if (fStatus != NULL)
			fStatus->SetText(B_TRANSLATE("Controls updated."));
		SaveSettings();
		if (close && fView != NULL)
			fView->MakeFocus();
		if (close && fInputWindow != NULL) {
			BWindow *window = fInputWindow;
			fInputWindow = NULL;
			window->PostMessage(B_QUIT_REQUESTED);
		}
		break;
	}
	case MSG_INPUT_CLOSED:
		fInputWindow = NULL;
		if (fView != NULL)
			fView->MakeFocus();
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
	case MSG_HELP:
		show_help();
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
