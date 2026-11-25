#include "DGenJoystick.h"

#ifdef WITH_JOYSTICK

#include "DGenLocale.h"

#include <Joystick.h>
#include <Message.h>
#include <String.h>

#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>

#include "DGenMessages.h"
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
	uint8 *captureAxes;
	uint8 *captureHats;
	bool *captureButtons;
	uint32_t lastPad;

	Device()
		: open(false),
		  axisCount(0),
		  hatCount(0),
		  buttonCount(0),
		  axes(NULL),
		  hats(NULL),
		  buttons(NULL),
		  captureAxes(NULL),
		  captureHats(NULL),
		  captureButtons(NULL),
		  lastPad(MD_PAD_UNTOUCHED)
	{
	}

	~Device()
	{
		Close();
		delete[] axes;
		delete[] hats;
		delete[] buttons;
		delete[] captureAxes;
		delete[] captureHats;
		delete[] captureButtons;
	}

	bool Allocate()
	{
		delete[] axes;
		delete[] hats;
		delete[] buttons;
		delete[] captureAxes;
		delete[] captureHats;
		delete[] captureButtons;
		axes = NULL;
		hats = NULL;
		buttons = NULL;
		captureAxes = NULL;
		captureHats = NULL;
		captureButtons = NULL;
		axisCount = stick.CountAxes();
		hatCount = stick.CountHats();
		buttonCount = stick.CountButtons();
		axes = new(std::nothrow) int16[axisCount > 0 ? axisCount : 1];
		hats = new(std::nothrow) uint8[hatCount > 0 ? hatCount : 1];
		buttons = new(std::nothrow) bool[buttonCount > 0 ? buttonCount : 1];
		captureAxes = new(std::nothrow) uint8[axisCount > 0 ? axisCount : 1];
		captureHats = new(std::nothrow) uint8[hatCount > 0 ? hatCount : 1];
		captureButtons = new(std::nothrow) bool[buttonCount > 0 ? buttonCount : 1];
		if (axes == NULL || hats == NULL || buttons == NULL ||
			captureAxes == NULL || captureHats == NULL || captureButtons == NULL)
			return false;
		memset(axes, 0, (size_t)(axisCount > 0 ? axisCount : 1) * sizeof(int16));
		memset(hats, 0, (size_t)(hatCount > 0 ? hatCount : 1) * sizeof(uint8));
		memset(buttons, 0, (size_t)(buttonCount > 0 ? buttonCount : 1) * sizeof(bool));
		memset(captureAxes, 0,
			(size_t)(axisCount > 0 ? axisCount : 1) * sizeof(uint8));
		memset(captureHats, 0,
			(size_t)(hatCount > 0 ? hatCount : 1) * sizeof(uint8));
		memset(captureButtons, 0,
			(size_t)(buttonCount > 0 ? buttonCount : 1) * sizeof(bool));
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
static std::mutex sCaptureLock;

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

static uint8 hat_direction(uint8 hat)
{
	switch (hat) {
	case 1:
	case 2:
		return JS_HAT_UP;
	case 3:
	case 4:
		return JS_HAT_RIGHT;
	case 5:
	case 6:
		return JS_HAT_DOWN;
	case 7:
	case 8:
		return JS_HAT_LEFT;
	default:
		return JS_HAT_CENTERED;
	}
}

void DGenJoystick::SnapshotCapture(Device *dev)
{
	for (int i = 0; i < dev->buttonCount; i++)
		dev->captureButtons[i] = dev->buttons[i];
	for (int i = 0; i < dev->axisCount; i++)
		dev->captureAxes[i] = (uint8)axis_direction(dev->axes[i]);
	for (int i = 0; i < dev->hatCount; i++)
		dev->captureHats[i] = hat_direction(dev->hats[i]);
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
	: fCapturePlayer(-1),
	  fCaptureActive(false),
	  fCapturePending(false)
{
	for (int i = 0; i < 2; i++)
		fDevices[i] = NULL;
}

DGenJoystick::~DGenJoystick()
{
	EndCapture();
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
			std::lock_guard<std::mutex> lock(sCaptureLock);
			if (fCaptureActive && fCapturePlayer == player) {
				fCaptureActive = false;
				fCapturePending = false;
				fCaptureTarget = BMessenger();
			}
			continue;
		}
		if (dev->axisCount > 0)
			dev->stick.GetAxisValues(dev->axes, 0);
		if (dev->hatCount > 0)
			dev->stick.GetHatValues(dev->hats, 0);
		if (dev->buttonCount > 0)
			dev->stick.GetButtonValues(dev->buttons, 0);

		CheckCapture(player, dev);

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

void DGenJoystick::CheckCapture(int player, Device *dev)
{
	BMessenger target;
	{
		std::lock_guard<std::mutex> lock(sCaptureLock);
		if (!fCaptureActive || !fCapturePending || fCapturePlayer != player)
			return;
		target = fCaptureTarget;
	}

	uint32 code = 0;
	for (int i = 0; i < dev->buttonCount && code == 0; i++) {
		if (dev->buttons[i] && !dev->captureButtons[i])
			code = JS_BUTTON(player, i);
	}
	for (int i = 0; i < dev->axisCount && code == 0; i++) {
		int direction = axis_direction(dev->axes[i]);
		if (direction != JS_AXIS_BETWEEN &&
			(uint8)direction != dev->captureAxes[i])
			code = JS_AXIS(player, i, direction);
	}
	for (int i = 0; i < dev->hatCount && code == 0; i++) {
		uint8 direction = hat_direction(dev->hats[i]);
		if (direction != JS_HAT_CENTERED && direction != dev->captureHats[i])
			code = JS_HAT(player, i, direction);
	}
	SnapshotCapture(dev);

	if (code == 0)
		return;

	{
		std::lock_guard<std::mutex> lock(sCaptureLock);
		if (!fCaptureActive || !fCapturePending || fCapturePlayer != player)
			return;
		fCaptureActive = false;
		fCapturePending = false;
		fCaptureTarget = BMessenger();
	}

	BMessage message(MSG_JOYSTICK_CAPTURE);
	message.AddInt32("player", player);
	message.AddInt32("code", (int32)code);
	target.SendMessage(&message);
}

void DGenJoystick::BeginCapture(const BMessenger &target, int player)
{
	if (player < 0 || player >= 2)
		return;

	std::lock_guard<std::mutex> lock(sCaptureLock);
	fCaptureTarget = target;
	fCapturePlayer = player;
	fCaptureActive = true;
	fCapturePending = true;
	Device *dev = fDevices[player];
	if (dev != NULL && dev->open)
		SnapshotCapture(dev);
}

void DGenJoystick::EndCapture()
{
	std::lock_guard<std::mutex> lock(sCaptureLock);
	fCaptureActive = false;
	fCapturePending = false;
	fCaptureTarget = BMessenger();
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

void dgen_haiku_joystick_begin_capture(const BMessenger &target, int player)
{
	sJoystick.BeginCapture(target, player);
}

void dgen_haiku_joystick_end_capture()
{
	sJoystick.EndCapture();
}

#endif
