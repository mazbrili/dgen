#ifndef __DGEN_MESSAGES_H__
#define __DGEN_MESSAGES_H__

enum {
	MSG_UPDATE_FRAME = 'dguf',
	MSG_STATUS_TEXT = 'dgst',
	MSG_OPEN_ROM = 'dgor',
	MSG_OPEN_RECENT = 'dgrc',
	MSG_REBUILD_RECENT = 'dgrb',
	MSG_CLOSE_CARTRIDGE = 'dgcc',
	MSG_FILE_SELECTED = 'dgfl',
	MSG_PAUSE = 'dgpa',
	MSG_RESET = 'dgrs',
	MSG_HARD_RESET = 'dghr',
	MSG_TOGGLE_FULLSCREEN = 'dgfu',
	MSG_SCALE_1X = 'dg1x',
	MSG_SCALE_2X = 'dg2x',
	MSG_SCALE_3X = 'dg3x',
	MSG_SET_SCALE = 'dgsc',
	MSG_TOGGLE_STATUS = 'dgss',
	MSG_CONFIGURE_INPUT = 'dgci',
	MSG_CALIBRATE = 'dgca',
	MSG_GAME_GENIE = 'dgge',
	MSG_GAME_GENIE_APPLY = 'dgga',
	MSG_GENIE_CLOSED = 'dggc',
	MSG_SAVE_STATE = 'dgvs',
	MSG_LOAD_STATE = 'dgld',
	MSG_ABOUT = 'dgab',
	MSG_SETTINGS = 'dgcf',
	MSG_QUIT = 'dgqt'
};

#endif
