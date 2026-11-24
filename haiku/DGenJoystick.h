#ifndef __DGEN_JOYSTICK_H__
#define __DGEN_JOYSTICK_H__

#ifdef WITH_JOYSTICK

class DGenJoystick {
public:
	DGenJoystick();
	~DGenJoystick();

	void Open();
	void Close();
	void Poll();

private:
	struct Device;
	Device *fDevices[2];
};

void dgen_haiku_joystick_open();
void dgen_haiku_joystick_close();
void dgen_haiku_joystick_poll();

#endif

#endif
