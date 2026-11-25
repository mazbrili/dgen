#ifndef __DGEN_JOYSTICK_H__
#define __DGEN_JOYSTICK_H__

#ifdef WITH_JOYSTICK

#include <Messenger.h>

class DGenJoystick {
public:
	DGenJoystick();
	~DGenJoystick();

	void Open();
	void Close();
	void Poll();
	void BeginCapture(const BMessenger &target, int player);
	void EndCapture();

private:
	struct Device;
	void CheckCapture(int player, Device *dev);
	void SnapshotCapture(Device *dev);

	Device *fDevices[2];
	BMessenger fCaptureTarget;
	int fCapturePlayer;
	bool fCaptureActive;
	bool fCapturePending;
};

void dgen_haiku_joystick_open();
void dgen_haiku_joystick_close();
void dgen_haiku_joystick_poll();
void dgen_haiku_joystick_begin_capture(const BMessenger &target, int player);
void dgen_haiku_joystick_end_capture();

#endif

#endif
