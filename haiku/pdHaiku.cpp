#include "pdHaiku.h"

#include <Bitmap.h>
#include <Message.h>
#include <OS.h>
#include <Window.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "DGenApp.h"
#include "DGenMessages.h"
#include "DGenView.h"
#include "DGenWindow.h"
#include "pd.h"

struct bmap mdscr;
unsigned char *mdpal = NULL;
struct sndinfo sndi;
bool pd_freeze = false;
const char *pd_options = "";

static DGenApp *sApp = NULL;
static std::atomic<bool> sAppReady(false);
static std::atomic<bool> sWindowReady(false);
static DGenWindow *sWindow = NULL;
static BBitmap *sBitmap = NULL;
static unsigned char *sFrameBuffer = NULL;
static int sVisibleWidth = 320;
static int sVisibleHeight = 224;
static std::vector<int16_t> sSoundBuffer;

static std::atomic<bool> sQuit(false);
static std::atomic<bool> sPaused(false);
static std::atomic<bool> sReset(false);
static std::atomic<bool> sHardReset(false);
static std::atomic<int> sScale(1);
static std::atomic<uint32_t> sPad[2];
static std::atomic<uint32_t> sPrevPad[2];

static std::mutex sPathLock;
static std::string sPendingPath;
static bool sHasPendingPath = false;

static md *sMegad = NULL;

static int32 app_thread(void *)
{
	sApp = new DGenApp();
	sWindow = new DGenWindow();
	sWindow->Show();
	sWindowReady.store(true);
	sAppReady.store(true);
	sApp->Run();
	return 0;
}

static void post_status(const char *text)
{
	BMessage message(MSG_STATUS_TEXT);
	message.AddString("text", text);
	if (sWindow != NULL)
		sWindow->PostMessage(&message);
}

unsigned long pd_usecs(void)
{
	return (unsigned long)system_time();
}

int pd_graphics_init(int, int want_pal, int)
{
	if (sWindow == NULL) {
		if (!sAppReady.load()) {
			thread_id thread = spawn_thread(app_thread, "DGen app",
				B_NORMAL_PRIORITY, NULL);
			resume_thread(thread);
		}
		while (!sWindowReady.load())
			snooze(1000);
	}

	for (int i = 0; i < 2; i++) {
		sPad[i].store(0);
		sPrevPad[i].store(0);
	}

	sVisibleWidth = 320;
	sVisibleHeight = want_pal ? 240 : 224;

	free(sFrameBuffer);
	sFrameBuffer = NULL;
	delete sBitmap;
	sBitmap = NULL;

	mdscr.w = sVisibleWidth + 16;
	mdscr.h = sVisibleHeight + 16;
	mdscr.pitch = mdscr.w * 4;
	mdscr.bpp = 32;
	sFrameBuffer = (unsigned char *)calloc(mdscr.h, mdscr.pitch);
	if (sFrameBuffer == NULL) {
		mdscr.data = NULL;
		post_status("Could not allocate the video frame buffer.");
		return 0;
	}
	mdscr.data = sFrameBuffer;

	sBitmap = new BBitmap(BRect(0, 0, sVisibleWidth - 1,
		sVisibleHeight - 1), B_RGB32);
	if (sBitmap->InitCheck() != B_OK) {
		post_status("Could not allocate the video bitmap.");
		return 0;
	}

	DGenView *view = sWindow->VideoView();
	if (view != NULL)
		view->SetFrameBitmap(sBitmap);

	mdpal = NULL;

	return 1;
}

int pd_graphics_reinit(int want_sound, int want_pal, int hz)
{
	return pd_graphics_init(want_sound, want_pal, hz);
}

void pd_graphics_palette_update()
{
}

void pd_graphics_update(bool update)
{
	if (!update || sWindow == NULL || sBitmap == NULL || sFrameBuffer == NULL)
		return;

	const unsigned char *source = sFrameBuffer + (mdscr.pitch * 8) + 16;
	unsigned char *target = (unsigned char *)sBitmap->Bits();
	const size_t rowBytes = (size_t)sVisibleWidth * 4;

	for (int y = 0; y < sVisibleHeight; y++)
		memcpy(target + ((size_t)y * sBitmap->BytesPerRow()),
			source + ((size_t)y * mdscr.pitch), rowBytes);

	sWindow->PostMessage(MSG_UPDATE_FRAME);
}

int pd_sound_init(long &, unsigned int &samples)
{
	sSoundBuffer.resize((size_t)samples * 2);
	sndi.lr = sSoundBuffer.empty() ? NULL : &sSoundBuffer[0];
	sndi.len = samples;
	return 1;
}

void pd_sound_deinit()
{
}

unsigned int pd_sound_rp()
{
	return 0;
}

unsigned int pd_sound_wp()
{
	return 0;
}

void pd_sound_write()
{
}

void pd_rc()
{
}

void pd_option(char, const char *)
{
}

void pd_help()
{
	printf("  Haiku front end: no platform-specific options.\n");
}

int pd_handle_events(md &megad)
{
	sMegad = &megad;

	if (sQuit.load())
		return 0;

	if (sHasPendingPath) {
		std::string path;
		{
			std::lock_guard<std::mutex> lock(sPathLock);
			path = sPendingPath;
			sPendingPath.clear();
			sHasPendingPath = false;
		}
		if (sMegad->load(path.c_str()) != 0)
			post_status("Could not open that ROM.");
		else
			post_status("ROM loaded.");
	}

	for (int i = 0; i < 2; i++) {
		uint32_t current = sPad[i].load();
		uint32_t previous = sPrevPad[i].load();
		megad.pad[i] = (megad.pad[i] & ~previous) | current;
		sPrevPad[i].store(current);
	}

	if (sReset.exchange(false) || sHardReset.exchange(false))
		megad.reset();

	pd_freeze = sPaused.load();
	return 1;
}

int pd_stopped()
{
	return sPaused.load() ? 1 : 0;
}

void pd_message(const char *fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	post_status(buffer);
}

void pd_clear_message()
{
	post_status("");
}

void pd_show_carthead(md &megad)
{
	pd_message("Cartridge: %s", megad.romname);
}

void pd_quit()
{
	sQuit.store(true);
	if (sWindow != NULL)
		sWindow->PostMessage(B_QUIT_REQUESTED);
}

DGenWindow *dgen_haiku_window(void)
{
	return sWindow;
}

void dgen_haiku_attach_window(DGenWindow *window)
{
	sWindow = window;
}

void dgen_haiku_set_pad(int player, uint32_t mask)
{
	if (player >= 0 && player < 2)
		sPad[player].store(mask);
}

void dgen_haiku_request_quit(void)
{
	sQuit.store(true);
}

void dgen_haiku_request_reset(bool hard)
{
	if (hard)
		sHardReset.store(true);
	else
		sReset.store(true);
}

void dgen_haiku_set_paused(bool paused)
{
	sPaused.store(paused);
}

void dgen_haiku_toggle_fullscreen(void)
{
	post_status("Full screen is not wired up yet.");
}

void dgen_haiku_set_scale(int scale)
{
	sScale.store(scale);
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "Scale %d× is not wired up yet.", scale);
	post_status(buffer);
}

void dgen_haiku_open_rom(const char *path)
{
	if (path == NULL)
		return;
	std::lock_guard<std::mutex> lock(sPathLock);
	sPendingPath = path;
	sHasPendingPath = true;
}
