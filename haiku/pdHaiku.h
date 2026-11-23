#ifndef __PD_HAIKU_H__
#define __PD_HAIKU_H__

#include <stdint.h>

class DGenWindow;

DGenWindow *dgen_haiku_window(void);
void dgen_haiku_attach_window(DGenWindow *window);
void dgen_haiku_set_pad(int player, uint32_t mask);
void dgen_haiku_request_quit(void);
void dgen_haiku_request_reset(bool hard);
void dgen_haiku_set_paused(bool paused);
void dgen_haiku_toggle_fullscreen(void);
void dgen_haiku_set_scale(int scale);
void dgen_haiku_get_emulation_settings(bool &sound, bool &pal, int &hz);
void dgen_haiku_set_emulation_settings(bool sound, bool pal, int hz);
void dgen_haiku_open_rom(const char *path);
void dgen_haiku_request_close_cartridge(void);
void dgen_haiku_request_save_state(int slot);
void dgen_haiku_request_load_state(int slot);
void dgen_haiku_request_game_genie(const char *code);

#endif
