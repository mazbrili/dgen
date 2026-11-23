#ifndef __DGEN_INPUT_H__
#define __DGEN_INPUT_H__

#include <Message.h>
#include <String.h>
#include <SupportDefs.h>

enum {
	DGEN_INPUT_UP = 0,
	DGEN_INPUT_DOWN,
	DGEN_INPUT_LEFT,
	DGEN_INPUT_RIGHT,
	DGEN_INPUT_A,
	DGEN_INPUT_B,
	DGEN_INPUT_C,
	DGEN_INPUT_X,
	DGEN_INPUT_Y,
	DGEN_INPUT_Z,
	DGEN_INPUT_MODE,
	DGEN_INPUT_START,
	DGEN_INPUT_ACTION_COUNT
};

struct DGenKey {
	uint32 modifiers;
	int32 key;
	int32 byteValue;
};

struct DGenInputMap {
	DGenKey keys[2][DGEN_INPUT_ACTION_COUNT];
};

const char *dgen_input_action_label(int index);
uint32_t dgen_input_action_mask(int index);
void dgen_input_defaults(DGenInputMap &map);
void dgen_input_load(const BMessage &settings, DGenInputMap &map);
void dgen_input_save(BMessage &settings, const DGenInputMap &map);
BString dgen_input_key_label(const DGenKey &key);
bool dgen_input_key_from_message(const BMessage &message, DGenKey &key);
bool dgen_input_key_matches(const DGenKey &binding, uint32 modifiers,
	int32 key, const char *bytes);

#endif
