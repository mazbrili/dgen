#ifndef __DGEN_VIEW_H__
#define __DGEN_VIEW_H__

#include <View.h>

#include <stdint.h>

class BBitmap;
class BMessage;

class DGenView : public BView {
public:
	DGenView(BRect frame);
	virtual void Draw(BRect updateRect);
	virtual void KeyDown(BMessage *message);
	virtual void KeyUp(BMessage *message);
	void SetFrameBitmap(BBitmap *bitmap);

private:
	BBitmap *fBitmap;
	uint32_t fPad;
};

#endif
