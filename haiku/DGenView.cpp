#include "DGenView.h"

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <Message.h>

#include "md.h"
#include "pdHaiku.h"

DGenView::DGenView(BRect frame)
	: BView(frame, "DGen video", B_FOLLOW_ALL, B_WILL_DRAW),
	  fBitmap(NULL),
	  fPad(MD_PAD_UNTOUCHED)
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
}

void DGenView::SetFrameBitmap(BBitmap *bitmap)
{
	fBitmap = bitmap;
	Invalidate();
}

void DGenView::KeyDown(BMessage *message)
{
	int32 key = 0;
	int32 modifiers = 0;
	const char *bytes = NULL;
	uint32_t mask = 0;

	message->FindInt32("key", &key);
	message->FindInt32("modifiers", &modifiers);
	message->FindString("bytes", &bytes);

	switch (key) {
	case B_UP_ARROW:
		mask = MD_UP_MASK;
		break;
	case B_DOWN_ARROW:
		mask = MD_DOWN_MASK;
		break;
	case B_LEFT_ARROW:
		mask = MD_LEFT_MASK;
		break;
	case B_RIGHT_ARROW:
		mask = MD_RIGHT_MASK;
		break;
	case B_RETURN:
		mask = MD_START_MASK;
		break;
	case B_ESCAPE:
		if (bytes != NULL && bytes[0] == 0x1b)
			dgen_haiku_request_quit();
		return;
	default:
		break;
	}

	if (mask == 0 && bytes != NULL) {
		switch (bytes[0]) {
		case 'w':
		case 'W':
			mask = MD_UP_MASK;
			break;
		case 's':
		case 'S':
			mask = MD_DOWN_MASK;
			break;
		case 'a':
		case 'A':
			mask = MD_LEFT_MASK;
			break;
		case 'd':
		case 'D':
			mask = MD_RIGHT_MASK;
			break;
		case 'z':
		case 'Z':
			mask = MD_A_MASK;
			break;
		case 'x':
		case 'X':
			mask = MD_B_MASK;
			break;
		case 'c':
		case 'C':
			mask = MD_C_MASK;
			break;
		default:
			break;
		}
	}

	if (mask == 0 && (modifiers & B_SHIFT_KEY) != 0)
		mask = MD_MODE_MASK;
	if (mask == 0)
		return;

	fPad |= mask;
	dgen_haiku_set_pad(0, fPad);
}

void DGenView::KeyUp(BMessage *message)
{
	int32 key = 0;
	const char *bytes = NULL;
	uint32_t mask = 0;

	message->FindInt32("key", &key);
	message->FindString("bytes", &bytes);

	switch (key) {
	case B_UP_ARROW:
		mask = MD_UP_MASK;
		break;
	case B_DOWN_ARROW:
		mask = MD_DOWN_MASK;
		break;
	case B_LEFT_ARROW:
		mask = MD_LEFT_MASK;
		break;
	case B_RIGHT_ARROW:
		mask = MD_RIGHT_MASK;
		break;
	case B_RETURN:
		mask = MD_START_MASK;
		break;
	default:
		break;
	}

	if (mask == 0 && bytes != NULL) {
		switch (bytes[0]) {
		case 'w':
		case 'W':
			mask = MD_UP_MASK;
			break;
		case 's':
		case 'S':
			mask = MD_DOWN_MASK;
			break;
		case 'a':
		case 'A':
			mask = MD_LEFT_MASK;
			break;
		case 'd':
		case 'D':
			mask = MD_RIGHT_MASK;
			break;
		case 'z':
		case 'Z':
			mask = MD_A_MASK;
			break;
		case 'x':
		case 'X':
			mask = MD_B_MASK;
			break;
		case 'c':
		case 'C':
			mask = MD_C_MASK;
			break;
		default:
			break;
		}
	}

	if (mask == 0)
		return;

	fPad &= ~mask;
	dgen_haiku_set_pad(0, fPad);
}
