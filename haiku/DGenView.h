#ifndef __DGEN_VIEW_H__
#define __DGEN_VIEW_H__

#include <View.h>

#include <stdint.h>

#include "DGenInput.h"

class BBitmap;
class BMessage;

class DGenView : public BView {
public:
	DGenView(BRect frame, const DGenInputMap *inputMap);
	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual void KeyDown(BMessage *message);
	virtual void KeyUp(BMessage *message);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage);
	virtual void MessageReceived(BMessage *message);
	void SetFrameBitmap(BBitmap *bitmap);
	void SetInputMap(const DGenInputMap *inputMap);

private:
	BBitmap *fBitmap;
	const DGenInputMap *fInputMap;
	uint32_t fPad[2];
	bool fDropTarget;
};

#endif
