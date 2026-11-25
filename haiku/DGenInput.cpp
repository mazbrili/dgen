#include "DGenInput.h"

#include <InterfaceDefs.h>

#include <cctype>
#include <cstdio>

#include "md.h"
#include "rc-vars.h"

static const char *kActionLabels[DGEN_INPUT_ACTION_COUNT] = {
	"Up", "Down", "Left", "Right", "A", "B", "C",
	"X", "Y", "Z", "Mode", "Start"
};

static const uint32 kJoystickDefaults[2][DGEN_INPUT_ACTION_COUNT] = {
	{
		JS_AXIS(0, 1, JS_AXIS_NEGATIVE),
		JS_AXIS(0, 1, JS_AXIS_POSITIVE),
		JS_AXIS(0, 0, JS_AXIS_NEGATIVE),
		JS_AXIS(0, 0, JS_AXIS_POSITIVE),
		JS_BUTTON(0, 0),
		JS_BUTTON(0, 3),
		JS_BUTTON(0, 1),
		JS_BUTTON(0, 6),
		JS_BUTTON(0, 4),
		JS_BUTTON(0, 5),
		JS_BUTTON(0, 9),
		JS_BUTTON(0, 8)
	},
	{
		JS_AXIS(1, 1, JS_AXIS_NEGATIVE),
		JS_AXIS(1, 1, JS_AXIS_POSITIVE),
		JS_AXIS(1, 0, JS_AXIS_NEGATIVE),
		JS_AXIS(1, 0, JS_AXIS_POSITIVE),
		JS_BUTTON(1, 0),
		JS_BUTTON(1, 3),
		JS_BUTTON(1, 1),
		JS_BUTTON(1, 6),
		JS_BUTTON(1, 4),
		JS_BUTTON(1, 5),
		JS_BUTTON(1, 9),
		JS_BUTTON(1, 8)
	}
};

static intptr_t *joystick_var(int player, int action)
{
	static intptr_t *vars[2][DGEN_INPUT_ACTION_COUNT] = {
		{
			pad1_up, pad1_down, pad1_left, pad1_right, pad1_a, pad1_b,
			pad1_c, pad1_x, pad1_y, pad1_z, pad1_mode, pad1_start
		},
		{
			pad2_up, pad2_down, pad2_left, pad2_right, pad2_a, pad2_b,
			pad2_c, pad2_x, pad2_y, pad2_z, pad2_mode, pad2_start
		}
	};
	if (player < 0 || player >= 2 || action < 0 ||
		action >= DGEN_INPUT_ACTION_COUNT)
		return NULL;
	return vars[player][action];
}

static int32 message_int32(const BMessage &message, const char *name)
{
	if (message.HasInt32(name))
		return message.FindInt32(name);
	return 0;
}

static const char *key_code_label(int32 key)
{
	switch (key) {
	case B_UP_ARROW:
		return "Up";
	case B_DOWN_ARROW:
		return "Down";
	case B_LEFT_ARROW:
		return "Left";
	case B_RIGHT_ARROW:
		return "Right";
	case B_RETURN:
		return "Enter";
	case B_TAB:
		return "Tab";
	case B_BACKSPACE:
		return "Backspace";
	case B_ESCAPE:
		return "Esc";
	case B_DELETE:
		return "Del";
	case B_INSERT:
		return "Ins";
	case B_HOME:
		return "Home";
	case B_END:
		return "End";
	case B_PAGE_UP:
		return "PgUp";
	case B_PAGE_DOWN:
		return "PgDn";
	case B_SPACE_BAR_KEY:
		return "Space";
	case B_LEFT_SHIFT_KEY:
	case B_RIGHT_SHIFT_KEY:
		return "Shift";
	default:
		return NULL;
	}
}

const char *dgen_input_action_label(int index)
{
	if (index < 0 || index >= DGEN_INPUT_ACTION_COUNT)
		return "";
	return kActionLabels[index];
}

uint32_t dgen_input_action_mask(int index)
{
	switch (index) {
	case DGEN_INPUT_UP:
		return MD_UP_MASK;
	case DGEN_INPUT_DOWN:
		return MD_DOWN_MASK;
	case DGEN_INPUT_LEFT:
		return MD_LEFT_MASK;
	case DGEN_INPUT_RIGHT:
		return MD_RIGHT_MASK;
	case DGEN_INPUT_A:
		return MD_A_MASK;
	case DGEN_INPUT_B:
		return MD_B_MASK;
	case DGEN_INPUT_C:
		return MD_C_MASK;
	case DGEN_INPUT_X:
		return MD_X_MASK;
	case DGEN_INPUT_Y:
		return MD_Y_MASK;
	case DGEN_INPUT_Z:
		return MD_Z_MASK;
	case DGEN_INPUT_MODE:
		return MD_MODE_MASK;
	case DGEN_INPUT_START:
		return MD_START_MASK;
	default:
		return 0;
	}
}

void dgen_input_defaults(DGenInputMap &map)
{
	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			map.keys[player][action].modifiers = 0;
			map.keys[player][action].key = 0;
			map.keys[player][action].byteValue = 0;
			map.joystick[player][action] = kJoystickDefaults[player][action];
		}
	}

	map.keys[0][DGEN_INPUT_UP].key = B_UP_ARROW;
	map.keys[0][DGEN_INPUT_DOWN].key = B_DOWN_ARROW;
	map.keys[0][DGEN_INPUT_LEFT].key = B_LEFT_ARROW;
	map.keys[0][DGEN_INPUT_RIGHT].key = B_RIGHT_ARROW;
	map.keys[0][DGEN_INPUT_A].byteValue = 'z';
	map.keys[0][DGEN_INPUT_B].byteValue = 'x';
	map.keys[0][DGEN_INPUT_C].byteValue = 'c';
	map.keys[0][DGEN_INPUT_X].byteValue = 'q';
	map.keys[0][DGEN_INPUT_Y].byteValue = 'w';
	map.keys[0][DGEN_INPUT_Z].byteValue = 'e';
	map.keys[0][DGEN_INPUT_MODE].modifiers = B_SHIFT_KEY;
	map.keys[0][DGEN_INPUT_START].key = B_RETURN;

	map.keys[1][DGEN_INPUT_UP].byteValue = 'i';
	map.keys[1][DGEN_INPUT_DOWN].byteValue = 'k';
	map.keys[1][DGEN_INPUT_LEFT].byteValue = 'j';
	map.keys[1][DGEN_INPUT_RIGHT].byteValue = 'l';
	map.keys[1][DGEN_INPUT_A].byteValue = '7';
	map.keys[1][DGEN_INPUT_B].byteValue = '8';
	map.keys[1][DGEN_INPUT_C].byteValue = '9';
	map.keys[1][DGEN_INPUT_X].byteValue = '0';
	map.keys[1][DGEN_INPUT_Y].byteValue = '-';
	map.keys[1][DGEN_INPUT_Z].byteValue = '=';
	map.keys[1][DGEN_INPUT_MODE].byteValue = 'm';
	map.keys[1][DGEN_INPUT_START].key = B_TAB;
}

void dgen_input_load_current_joystick(DGenInputMap &map)
{
	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			intptr_t *var = joystick_var(player, action);
			if (var != NULL)
				map.joystick[player][action] = (uint32)*var;
		}
	}
}

void dgen_input_apply_joystick(const DGenInputMap &map)
{
	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			intptr_t *var = joystick_var(player, action);
			if (var != NULL)
				*var = map.joystick[player][action];
		}
	}
}

void dgen_input_load(const BMessage &settings, DGenInputMap &map)
{
	dgen_input_defaults(map);

	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			int index = player * DGEN_INPUT_ACTION_COUNT + action;
			if (settings.HasInt32("modifiers", index))
				map.keys[player][action].modifiers =
					(uint32)settings.FindInt32("modifiers", index);
			if (settings.HasInt32("key", index))
				map.keys[player][action].key = settings.FindInt32("key", index);
			if (settings.HasInt32("byteValue", index))
				map.keys[player][action].byteValue =
					settings.FindInt32("byteValue", index);
			if (settings.HasInt32("joy", index))
				map.joystick[player][action] =
					(uint32)settings.FindInt32("joy", index);
		}
	}
}

void dgen_input_save(BMessage &settings, const DGenInputMap &map)
{
	settings.RemoveName("modifiers");
	settings.RemoveName("key");
	settings.RemoveName("byteValue");
	settings.RemoveName("joy");

	for (int player = 0; player < 2; player++) {
		for (int action = 0; action < DGEN_INPUT_ACTION_COUNT; action++) {
			settings.AddInt32("modifiers",
				(int32)map.keys[player][action].modifiers);
			settings.AddInt32("key", map.keys[player][action].key);
			settings.AddInt32("byteValue",
				map.keys[player][action].byteValue);
			settings.AddInt32("joy", (int32)map.joystick[player][action]);
		}
	}
}

BString dgen_input_key_label(const DGenKey &key)
{
	BString label;
	if ((key.modifiers & B_SHIFT_KEY) != 0)
		label = "Shift+";

	const char *code = key_code_label(key.key);
	if (code != NULL) {
		label += code;
		return label;
	}
	if (key.byteValue >= 32 && key.byteValue < 127) {
		char text[2] = { (char)key.byteValue, '\0' };
		label += text;
		return label;
	}
	if (label.Length() > 0)
		label.Truncate(label.Length() - 1);
	else
		label = "Unassigned";
	return label;
}

BString dgen_input_joystick_label(uint32 code)
{
	if (code == 0)
		return BString("Unassigned");

	BString label;
	label.SetToFormat("J%d ", JS_GET_IDENTIFIER(code) + 1);
	if (JS_IS_BUTTON(code)) {
		label << "B" << (JS_GET_BUTTON(code) + 1);
		return label;
	}
	if (JS_IS_AXIS(code)) {
		label << "A" << (JS_GET_AXIS(code) + 1);
		int direction = JS_GET_AXIS_DIR(code);
		if (direction == JS_AXIS_NEGATIVE)
			label << "-";
		else if (direction == JS_AXIS_POSITIVE)
			label << "+";
		return label;
	}
	if (JS_IS_HAT(code)) {
		const char *directions[] = { "center", "up", "right", "down", "left" };
		int direction = JS_GET_HAT_DIR(code);
		label << "H" << (JS_GET_HAT(code) + 1);
		if (direction >= 0 && direction < 5)
			label << " " << directions[direction];
		return label;
	}
	return BString("Invalid");
}

bool dgen_input_key_from_message(const BMessage &message, DGenKey &key)
{
	int32 modifiers = 0;
	int32 rawKey = 0;
	int32 byteValue = 0;
	const char *bytes = message.FindString("bytes");

	modifiers = message_int32(message, "modifiers");
	rawKey = message_int32(message, "key");
	byteValue = message_int32(message, "byteValue");
	if (rawKey == B_ESCAPE || (bytes != NULL && bytes[0] == 0x1b))
		return false;
	if (byteValue == 0 && bytes != NULL && bytes[0] >= 32 && bytes[0] < 127)
		byteValue = std::tolower((unsigned char)bytes[0]);

	key.modifiers = (uint32)(modifiers & B_SHIFT_KEY);
	if (key_code_label(rawKey) != NULL || byteValue == 0) {
		key.key = rawKey;
		key.byteValue = 0;
	} else {
		key.key = 0;
		key.byteValue = byteValue;
	}
	if (byteValue == 0 && key_code_label(rawKey) == NULL)
		key.key = 0;
	return true;
}

bool dgen_input_key_matches(const DGenKey &binding, uint32 modifiers,
	int32 key, const char *bytes)
{
	bool shiftKey = key == B_LEFT_SHIFT_KEY || key == B_RIGHT_SHIFT_KEY;
	if (binding.key == 0 && binding.byteValue == 0)
		return (binding.modifiers & B_SHIFT_KEY) != 0 && shiftKey;
	if ((modifiers & B_SHIFT_KEY) != (binding.modifiers & B_SHIFT_KEY))
		return false;
	if (binding.key != 0)
		return key == binding.key;
	return bytes != NULL && bytes[0] != '\0' &&
		std::tolower((unsigned char)bytes[0]) == binding.byteValue;
}

bool dgen_input_keys_equal(const DGenKey &a, const DGenKey &b)
{
	return a.modifiers == b.modifiers && a.key == b.key &&
		a.byteValue == b.byteValue;
}

void dgen_input_clear_conflicts(DGenInputMap &map, int player, int action)
{
	DGenKey empty = { 0, 0, 0 };
	for (int otherPlayer = 0; otherPlayer < 2; otherPlayer++) {
		for (int otherAction = 0;
			otherAction < DGEN_INPUT_ACTION_COUNT; otherAction++) {
			if (otherPlayer == player && otherAction == action)
				continue;
			if (dgen_input_keys_equal(map.keys[otherPlayer][otherAction],
					map.keys[player][action]))
				map.keys[otherPlayer][otherAction] = empty;
		}
	}
}

void dgen_input_clear_joystick_conflicts(DGenInputMap &map, int player,
	int action, uint32 code)
{
	if (code == 0)
		return;
	for (int otherPlayer = 0; otherPlayer < 2; otherPlayer++) {
		for (int otherAction = 0;
			otherAction < DGEN_INPUT_ACTION_COUNT; otherAction++) {
			if (otherPlayer == player && otherAction == action)
				continue;
			if (map.joystick[otherPlayer][otherAction] == code)
				map.joystick[otherPlayer][otherAction] = 0;
		}
	}
}
