#include "DGenJoystick.h"

#ifdef WITH_JOYSTICK

#include "DGenLocale.h"

#include <Joystick.h>
#include <String.h>

#include <cstdio>
#include <cstring>
#include <new>

#include "md.h"
#include "pdHaiku.h"
#include "rc-vars.h"
#include "rc.h"

struct DGenJoystick::Device {
	BJoystick stick;
	bool open;
	int axisCount;
	int hatCount;
	int buttonCount;
	int16 *axes;
	uint8 *hats;
	bool *buttons;
	uint32_t lastPad;

	Device()
		: open(false),
		  axisCount(0),
		  hatCount(0),
		  buttonCount(0),
		  axes(NULL),
		  hats(NULL),
		  buttons(NULL),
		  lastPad(MD_PAD_UNTOUCHED)
	{
	}

	~Device()
	{
		Close();
		delete[] axes;
		delete[] hats;
		delete[] buttons;
	}

	bool Allocate()
	{
		delete[] axes;
		delete[] hats;
		delete[] buttons;
		axes = NULL;
		hats = NULL;
		buttons = NULL;
		axisCount = stick.CountAxes();
		hatCount = stick.CountHats();
		buttonCount = stick.CountButtons();
		axes = new(std::nothrow) int16[axisCount > 0 ? axisCount : 1];
		hats = new(std::nothrow) uint8[hatCount > 0 ? hatCount : 1];
		buttons = new(std::nothrow) bool[buttonCount > 0 ? buttonCount : 1];
		if (axes == NULL || hats == NULL || buttons == NULL)
			return false;
		memset(axes, 0, (size_t)(axisCount > 0 ? axisCount : 1) * sizeof(int16));
		memset(hats, 0, (size_t)(hatCount > 0 ? hatCount : 1) * sizeof(uint8));
		memset(buttons, 0, (size_t)(buttonCount > 0 ? buttonCount : 1) * sizeof(bool));
		return true;
	}

	void Close()
	{
		if (open)
			stick.Close();
		open = false;
	}
};

static DGenJoystick sJoystick;
static bool sJoystickOpened = false;

static int axis_direction(int16 value)
{
	if (value <= -16384)
		return JS_AXIS_NEGATIVE;
	if (value >= 16384)
		return JS_AXIS_POSITIVE;
	return JS_AXIS_BETWEEN;
}

static bool hat_has_direction(uint8 hat, int dir)
{
	switch (dir) {
	case JS_HAT_UP:
		return hat == 1 || hat == 2 || hat == 8;
	case JS_HAT_RIGHT:
		return hat == 2 || hat == 3 || hat == 4;
	case JS_HAT_DOWN:
		return hat == 4 || hat == 5 || hat == 6;
	case JS_HAT_LEFT:
		return hat == 6 || hat == 7 || hat == 8;
	default:
		return false;
	}
}

static bool hat_direction_set(uint8 hat, uint32_t code)
{
	return hat_has_direction(hat, JS_GET_HAT_DIR(code));
}

static bool binding_for_pad(int player, uint32_t mask, uint32_t &code)
{
#define MAP(field, bit) \
	if (mask == (bit)) { \
		code = (uint32_t)field[RCBJ]; \
		return code != 0; \
	}
	if (player == 0) {
		MAP(pad1_up, MD_UP_MASK)
		MAP(pad1_down, MD_DOWN_MASK)
		MAP(pad1_left, MD_LEFT_MASK)
		MAP(pad1_right, MD_RIGHT_MASK)
		MAP(pad1_a, MD_A_MASK)
		MAP(pad1_b, MD_B_MASK)
		MAP(pad1_c, MD_C_MASK)
		MAP(pad1_x, MD_X_MASK)
		MAP(pad1_y, MD_Y_MASK)
		MAP(pad1_z, MD_Z_MASK)
		MAP(pad1_mode, MD_MODE_MASK)
		MAP(pad1_start, MD_START_MASK)
	} else if (player == 1) {
		MAP(pad2_up, MD_UP_MASK)
		MAP(pad2_down, MD_DOWN_MASK)
		MAP(pad2_left, MD_LEFT_MASK)
		MAP(pad2_right, MD_RIGHT_MASK)
		MAP(pad2_a, MD_A_MASK)
		MAP(pad2_b, MD_B_MASK)
		MAP(pad2_c, MD_C_MASK)
		MAP(pad2_x, MD_X_MASK)
		MAP(pad2_y, MD_Y_MASK)
		MAP(pad2_z, MD_Z_MASK)
		MAP(pad2_mode, MD_MODE_MASK)
		MAP(pad2_start, MD_START_MASK)
	}
#undef MAP
	return false;
}

DGenJoystick::DGenJoystick()
{
	for (int i = 0; i < 2; i++)
		fDevices[i] = NULL;
}

DGenJoystick::~DGenJoystick()
{
	Close();
}

void DGenJoystick::Open()
{
	Close();
	if (!dgen_joystick)
		return;

	BJoystick probe;
	int32 count = probe.CountDevices();
	int opened = 0;
	for (int32 i = 0; i < count && opened < 2; i++) {
		char name[B_OS_NAME_LENGTH];
		if (probe.GetDeviceName(i, name, sizeof(name)) != B_OK)
			continue;
		Device *dev = new Device;
		status_t status = dev->stick.Open(name, true);
		if (status < B_OK) {
			fprintf(stderr, "joystick: open %s failed: %s\n", name,
				strerror(status));
			delete dev;
			continue;
		}
		dev->stick.EnterEnhancedMode();
		if (dev->stick.CountSticks() <= 0 || !dev->Allocate()) {
			fprintf(stderr, "joystick: %s has no usable sticks\n", name);
			delete dev;
			continue;
		}
		dev->open = true;
		fprintf(stderr, "joystick: opened %s (%d axes, %d hats, %d buttons)\n",
			name, dev->axisCount, dev->hatCount, dev->buttonCount);
		fDevices[opened++] = dev;
	}
	if (opened == 0)
		fprintf(stderr, "joystick: no BJoystick devices found\n");
}

void DGenJoystick::Close()
{
	for (int i = 0; i < 2; i++) {
		if (fDevices[i] == NULL)
			continue;
		delete fDevices[i];
		fDevices[i] = NULL;
	}
}

void DGenJoystick::Poll()
{
	if (!dgen_joystick)
		return;

	for (int player = 0; player < 2; player++) {
		Device *dev = fDevices[player];
		if (dev == NULL || !dev->open)
			continue;
		if (dev->stick.Update() != B_OK) {
			dev->Close();
			dgen_haiku_set_joystick_pad(player, MD_PAD_UNTOUCHED);
			continue;
		}
		if (dev->axisCount > 0)
			dev->stick.GetAxisValues(dev->axes, 0);
		if (dev->hatCount > 0)
			dev->stick.GetHatValues(dev->hats, 0);
		if (dev->buttonCount > 0)
			dev->stick.GetButtonValues(dev->buttons, 0);

		uint32_t pad = dev->lastPad;
		static const uint32_t masks[] = {
			MD_UP_MASK, MD_DOWN_MASK, MD_LEFT_MASK, MD_RIGHT_MASK,
			MD_A_MASK, MD_B_MASK, MD_C_MASK, MD_X_MASK, MD_Y_MASK, MD_Z_MASK,
			MD_MODE_MASK, MD_START_MASK
		};
		for (unsigned m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
			uint32_t code;
			if (!binding_for_pad(player, masks[m], code))
				continue;
			int id = JS_GET_IDENTIFIER(code);
			if (id != player)
				continue;
			bool pressed = false;
			if (JS_IS_BUTTON(code)) {
				int b = JS_GET_BUTTON(code);
				if (b >= 0 && b < dev->buttonCount)
					pressed = dev->buttons[b];
			} else if (JS_IS_AXIS(code)) {
				int axis = JS_GET_AXIS(code);
				int dir = JS_GET_AXIS_DIR(code);
				if (axis >= 0 && axis < dev->axisCount)
					pressed = axis_direction(dev->axes[axis]) == dir;
			} else if (JS_IS_HAT(code)) {
				int hat = JS_GET_HAT(code);
				if (hat >= 0 && hat < dev->hatCount)
					pressed = hat_direction_set(dev->hats[hat], code);
			}
			if (pressed)
				pad &= ~masks[m];
			else
				pad |= masks[m];
		}
		if (pad != dev->lastPad) {
			dev->lastPad = pad;
			dgen_haiku_set_joystick_pad(player, pad);
		}
	}
}

void dgen_haiku_joystick_open()
{
	if (sJoystickOpened)
		return;
	sJoystick.Open();
	sJoystickOpened = true;
}

void dgen_haiku_joystick_close()
{
	if (!sJoystickOpened)
		return;
	sJoystick.Close();
	sJoystickOpened = false;
}

void dgen_haiku_joystick_poll()
{
	if (!sJoystickOpened)
		return;
	sJoystick.Poll();
}

#endif
