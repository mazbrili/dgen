#include "DGenView.h"

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Window.h>

#include "md.h"
#include "pdHaiku.h"

DGenView::DGenView(BRect frame, const DGenInputMap *inputMap)
	: BView(frame, "DGen video", B_FOLLOW_ALL, B_WILL_DRAW),
	  fBitmap(NULL),
	  fInputMap(inputMap),
	  fPad{ MD_PAD_UNTOUCHED, MD_PAD_UNTOUCHED },
	  fDropTarget(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void DGenView::Draw(BRect)
{
	if (fBitmap == NULL) {
		SetHighColor(0, 0, 0);
		FillRect(Bounds());
		return;
	}
	DrawBitmap(fBitmap, fBitmap->Bounds(), Bounds());
	if (fDropTarget) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		SetPenSize(3);
		StrokeRect(Bounds().InsetBySelf(2, 2));
		SetPenSize(1);
	}
}

void DGenView::MouseDown(BPoint)
{
	if (Window() != NULL)
		MakeFocus();
}

void DGenView::MouseMoved(BPoint, uint32 code, const BMessage *dragMessage)
{
	bool accepting = dragMessage != NULL && dragMessage->HasRef("refs") &&
		code != B_EXITED_VIEW && code != B_OUTSIDE_VIEW;
	if (accepting != fDropTarget) {
		fDropTarget = accepting;
		Invalidate();
	}
}

void DGenView::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		fDropTarget = false;
		Invalidate();
		if (Window() != NULL)
			Window()->PostMessage(message);
		return;
	}
	BView::MessageReceived(message);
}

void DGenView::SetFrameBitmap(BBitmap *bitmap)
{
	fBitmap = bitmap;
	Invalidate();
}

void DGenView::SetInputMap(const DGenInputMap *inputMap)
{
	fInputMap = inputMap;
}

void DGenView::KeyDown(BMessage *message)
{
	int32 key = 0;
	int32 modifiers = 0;
	const char *bytes = NULL;

	message->FindInt32("key", &key);
	message->FindInt32("modifiers", &modifiers);
	message->FindString("bytes", &bytes);

	if (key == B_ESCAPE && bytes != NULL && bytes[0] == 0x1b) {
		dgen_haiku_request_quit();
		return;
	}

	DGenInputMap defaults;
	const DGenInputMap *map = fInputMap;
	if (map == NULL) {
		dgen_input_defaults(defaults);
		map = &defaults;
	}

	bool changed = false;
	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			if (!dgen_input_key_matches(map->keys[player][action],
					(uint32)modifiers, key, bytes))
				continue;
			uint32_t mask = dgen_input_action_mask(action);
			if (mask == 0)
				continue;
			fPad[player] |= mask;
			dgen_haiku_set_pad(player, fPad[player]);
			changed = true;
		}
	}
	(void)changed;
}

void DGenView::KeyUp(BMessage *message)
{
	int32 key = 0;
	int32 modifiers = 0;
	const char *bytes = NULL;

	message->FindInt32("key", &key);
	message->FindInt32("modifiers", &modifiers);
	message->FindString("bytes", &bytes);

	DGenInputMap defaults;
	const DGenInputMap *map = fInputMap;
	if (map == NULL) {
		dgen_input_defaults(defaults);
		map = &defaults;
	}

	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			if (!dgen_input_key_matches(map->keys[player][action],
					(uint32)modifiers, key, bytes))
				continue;
			uint32_t mask = dgen_input_action_mask(action);
			if (mask == 0)
				continue;
			fPad[player] &= ~mask;
			dgen_haiku_set_pad(player, fPad[player]);
		}
	}
}
