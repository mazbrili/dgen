#include "pdHaiku.h"

#include "DGenLocale.h"

#include <Bitmap.h>
#include <ByteOrder.h>
#include <MediaDefs.h>
#include <Message.h>
#include <OS.h>
#include <SoundPlayer.h>
#include <Window.h>

#include <algorithm>
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
#include "DGenWindow.h"
#include "pd.h"
#include "rc-vars.h"

struct bmap mdscr;
unsigned char *mdpal = NULL;
struct sndinfo sndi;
bool pd_freeze = false;
const char *pd_options = "";

extern int slot;
void md_save(md &megad);
void md_load(md &megad);

static DGenApp *sApp = NULL;
static std::atomic<bool> sAppReady(false);
static std::atomic<bool> sWindowReady(false);
static DGenWindow *sWindow = NULL;
static BBitmap *sBitmap = NULL;
static unsigned char *sFrameBuffer = NULL;
static int sVisibleWidth = 320;
static int sVisibleHeight = 224;
static std::vector<int16_t> sSoundBuffer;
static BSoundPlayer *sSoundPlayer = NULL;
static std::vector<int16_t> sSoundRing;
static size_t sSoundRingFrames = 0;
static size_t sSoundRingMask = 0;
static std::atomic<size_t> sSoundRead(0);
static std::atomic<size_t> sSoundWrite(0);

static std::atomic<bool> sQuit(false);
static std::atomic<bool> sPaused(false);
static std::atomic<bool> sReset(false);
static std::atomic<bool> sHardReset(false);
static std::atomic<int> sScale(1);
static std::atomic<bool> sSoundEnabled(true);
static std::atomic<bool> sPalEnabled(false);
static std::atomic<int> sHz(60);
static std::atomic<bool> sSettingsDirty(false);
static std::atomic<bool> sSoundChanged(false);
static std::atomic<bool> sVideoChanged(false);
static std::atomic<uint32_t> sPad[2];
static std::atomic<uint32_t> sPrevPad[2];

static std::mutex sPathLock;
static std::string sPendingPath;
static bool sHasPendingPath = false;
static std::mutex sActionLock;
static int sPendingSlot = 0;
static bool sPendingSave = false;
static bool sPendingLoad = false;
static bool sPendingClose = false;
static std::string sPendingGenie;
static bool sHasPendingGenie = false;

static md *sMegad = NULL;

struct EmulatorThreadData {
	int (*entry)(void *);
	void *arg;
	int result;
};

static int32 emulator_thread(void *data)
{
	EmulatorThreadData *threadData = (EmulatorThreadData *)data;
	threadData->result = threadData->entry(threadData->arg);
	dgen_haiku_request_quit();
	if (sWindow != NULL)
		sWindow->PostMessage(B_QUIT_REQUESTED);
	return 0;
}

int pd_run(int (*entry)(void *), void *arg)
{
	sApp = new DGenApp();
	sWindow = new DGenWindow();
	sWindow->Show();
	sWindowReady.store(true);
	sAppReady.store(true);
	fprintf(stderr, "dgen: native Haiku UI ready\n");

	EmulatorThreadData data = { entry, arg, 0 };
	thread_id thread = spawn_thread(emulator_thread, "DGen emulator",
		B_NORMAL_PRIORITY, &data);
	if (thread < B_OK) {
		fprintf(stderr, "dgen: could not start emulator thread: %s\n",
			strerror((int)thread));
		sApp->Quit();
		return 1;
	}
	resume_thread(thread);
	fprintf(stderr, "dgen: emulator thread started\n");
	sApp->Run();
	status_t status = B_OK;
	wait_for_thread(thread, &status);
	return data.result;
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

int pd_graphics_init(int want_sound, int want_pal, int hz)
{
	while (!sWindowReady.load())
		snooze(1000);

	sSoundEnabled.store(want_sound != 0);
	sPalEnabled.store(want_pal != 0);
	sHz.store(hz);

	for (int i = 0; i < 2; i++) {
		sPad[i].store(0);
		sPrevPad[i].store(0);
	}

	sVisibleWidth = 320;
	sVisibleHeight = want_pal ? 240 : 224;

	free(sFrameBuffer);
	sFrameBuffer = NULL;
	BBitmap *oldBitmap = sBitmap;
	sBitmap = NULL;

	mdscr.w = sVisibleWidth + 16;
	mdscr.h = sVisibleHeight + 16;
	mdscr.pitch = mdscr.w * 4;
	mdscr.bpp = 32;
	sFrameBuffer = (unsigned char *)calloc(mdscr.h, mdscr.pitch);
	if (sFrameBuffer == NULL) {
		mdscr.data = NULL;
		post_status(B_TRANSLATE("Could not allocate the video frame buffer."));
		return 0;
	}
	mdscr.data = sFrameBuffer;

	sBitmap = new BBitmap(BRect(0, 0, sVisibleWidth - 1,
		sVisibleHeight - 1), B_RGB32);
	if (sBitmap->InitCheck() != B_OK) {
		post_status(B_TRANSLATE("Could not allocate the video bitmap."));
		delete sBitmap;
		sBitmap = NULL;
		return 0;
	}

	BMessage message(MSG_SET_FRAME_BITMAP);
	message.AddPointer("bitmap", sBitmap);
	message.AddPointer("old", oldBitmap);
	if (sWindow != NULL)
		sWindow->PostMessage(&message);

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

static void sound_callback(void *, void *buffer, size_t size,
	const media_raw_audio_format &format)
{
	int16_t *out = (int16_t *)buffer;
	const size_t bytesPerSample = format.format & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	const size_t frames = size / (format.channel_count * bytesPerSample);
	const bool swap = format.byte_order !=
		(B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN);

	if (format.channel_count != 2 || bytesPerSample != 2 || sSoundRingFrames == 0) {
		memset(buffer, 0, size);
		return;
	}

	const size_t read = sSoundRead.load();
	const size_t write = sSoundWrite.load();
	const size_t available = write > read ? std::min(frames, write - read) : 0;
	for (size_t i = 0; i < frames; i++) {
		if (read + i >= write) {
			out[i * 2] = 0;
			out[i * 2 + 1] = 0;
			continue;
		}
		const size_t index = ((read + i) & sSoundRingMask) * 2;
		int16_t left = sSoundRing[index];
		int16_t right = sSoundRing[index + 1];
		if (swap) {
			left = (int16_t)((uint16_t(left) >> 8) | (uint16_t(left) << 8));
			right = (int16_t)((uint16_t(right) >> 8) | (uint16_t(right) << 8));
		}
		out[i * 2] = left;
		out[i * 2 + 1] = right;
	}
	sSoundRead.store(read + available);
}

int pd_sound_init(long &freq, unsigned int &samples)
{
	pd_sound_deinit();

	sSoundBuffer.assign((size_t)samples * 2, 0);
	sndi.lr = sSoundBuffer.empty() ? NULL : &sSoundBuffer[0];
	sndi.len = (long)(freq / (dgen_hz > 0 ? dgen_hz : 60));

	size_t ringFrames = 256;
	while (ringFrames < (size_t)samples + (size_t)sndi.len + 256)
		ringFrames <<= 1;
	sSoundRingFrames = ringFrames;
	sSoundRingMask = ringFrames - 1;
	sSoundRing.assign(ringFrames * 2, 0);
	sSoundRead.store(0);
	sSoundWrite.store(0);

	media_raw_audio_format format;
	format.frame_rate = (float)freq;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_HOST_IS_LENDIAN ?
		B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	format.buffer_size = 0;

	fprintf(stderr, "dgen: creating BSoundPlayer\n");
	sSoundPlayer = new BSoundPlayer(&format, "DGen", sound_callback);
	status_t status = sSoundPlayer->InitCheck();
	if (status == B_OK)
		status = sSoundPlayer->Start();
	if (status != B_OK) {
		fprintf(stderr, "sound: could not start BSoundPlayer: %s\n",
			strerror(status));
		delete sSoundPlayer;
		sSoundPlayer = NULL;
		sSoundRing.clear();
		sSoundRingFrames = 0;
		sSoundRingMask = 0;
		return 0;
	}

	fprintf(stderr, "sound: %ldHz, stereo 16-bit, %u buffer samples\n",
		freq, samples);
	return 1;
}

void pd_sound_deinit()
{
	if (sSoundPlayer != NULL) {
		sSoundPlayer->Stop();
		delete sSoundPlayer;
		sSoundPlayer = NULL;
	}
	sSoundRing.clear();
	sSoundRingFrames = 0;
	sSoundRingMask = 0;
	sSoundRead.store(0);
	sSoundWrite.store(0);
	sSoundBuffer.clear();
	sndi.lr = NULL;
	sndi.len = 0;
}

unsigned int pd_sound_rp()
{
	return (unsigned int)(sSoundRead.load() & sSoundRingMask);
}

unsigned int pd_sound_wp()
{
	return (unsigned int)(sSoundWrite.load() & sSoundRingMask);
}

void pd_sound_write()
{
	if (sSoundPlayer == NULL || sndi.lr == NULL || sndi.len <= 0 ||
		sSoundRingFrames == 0)
		return;

	const size_t write = sSoundWrite.load();
	const size_t read = sSoundRead.load();
	const size_t used = write - read;
	const size_t frames = std::min((size_t)sndi.len, sSoundRingFrames - used);
	for (size_t i = 0; i < frames; i++) {
		const size_t index = ((write + i) & sSoundRingMask) * 2;
		sSoundRing[index] = sndi.lr[i * 2];
		sSoundRing[index + 1] = sndi.lr[i * 2 + 1];
	}
	sSoundWrite.store(write + frames);
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

void pd_rom_loaded(const char *path)
{
	if (path == NULL || path[0] == '\0')
		return;
	if (dgen_app_instance != NULL)
		dgen_app_instance->AddRecentFile(path);
	if (sWindow != NULL)
		sWindow->PostMessage(MSG_REBUILD_RECENT);
}

void dgen_haiku_request_close_cartridge(void)
{
	std::lock_guard<std::mutex> lock(sActionLock);
	sPendingClose = true;
}

void dgen_haiku_request_save_state(int stateSlot)
{
	std::lock_guard<std::mutex> lock(sActionLock);
	sPendingSlot = stateSlot < 0 ? 0 : (stateSlot > 9 ? 9 : stateSlot);
	sPendingSave = true;
}

void dgen_haiku_request_load_state(int stateSlot)
{
	std::lock_guard<std::mutex> lock(sActionLock);
	sPendingSlot = stateSlot < 0 ? 0 : (stateSlot > 9 ? 9 : stateSlot);
	sPendingLoad = true;
}

void dgen_haiku_request_game_genie(const char *code)
{
	if (code == NULL)
		return;
	std::lock_guard<std::mutex> lock(sActionLock);
	sPendingGenie = code;
	sHasPendingGenie = true;
}

void dgen_haiku_get_emulation_settings(bool &sound, bool &pal, int &hz)
{
	sound = sSoundEnabled.load();
	pal = sPalEnabled.load();
	hz = sHz.load();
}

void dgen_haiku_set_emulation_settings(bool sound, bool pal, int hz)
{
	if (hz != 50 && hz != 60)
		hz = pal ? 50 : 60;
	bool soundChanged = sound != sSoundEnabled.load();
	bool videoChanged = pal != sPalEnabled.load() || hz != sHz.load();
	sSoundEnabled.store(sound);
	sPalEnabled.store(pal);
	sHz.store(hz);
	if (soundChanged)
		sSoundChanged.store(true);
	if (videoChanged)
		sVideoChanged.store(true);
	if (soundChanged || videoChanged)
		sSettingsDirty.store(true);
}

static void apply_pending_settings()
{
	if (!sSettingsDirty.exchange(false))
		return;

	dgen_sound = sSoundEnabled.load() ? 1 : 0;
	dgen_pal = sPalEnabled.load() ? 1 : 0;
	dgen_hz = sHz.load();

	if (sVideoChanged.exchange(false))
		pd_graphics_reinit((int)dgen_sound, (int)dgen_pal, (int)dgen_hz);

	if (sSoundChanged.exchange(false)) {
		if (dgen_sound) {
			long rate = dgen_soundrate;
			if (dgen_soundsegs < 0)
				dgen_soundsegs = 0;
			unsigned int samples =
				(unsigned int)(dgen_soundsegs * (rate / dgen_hz));
			pd_sound_init(rate, samples);
		} else {
			pd_sound_deinit();
		}
	}

	post_status(B_TRANSLATE("Settings applied."));
}

int pd_handle_events(md &megad)
{
	apply_pending_settings();
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
		if (sMegad->load(path.c_str()) != 0) {
			post_status(B_TRANSLATE("Could not open that ROM."));
		} else {
			pd_rom_loaded(path.c_str());
			post_status(B_TRANSLATE("ROM loaded."));
		}
	}

	bool saveState = false;
	bool loadState = false;
	bool closeCartridge = false;
	bool applyGenie = false;
	int stateSlot = 0;
	std::string genieCode;
	{
		std::lock_guard<std::mutex> lock(sActionLock);
		saveState = sPendingSave;
		loadState = sPendingLoad;
		closeCartridge = sPendingClose;
		applyGenie = sHasPendingGenie;
		stateSlot = sPendingSlot;
		genieCode = sPendingGenie;
		sPendingSave = false;
		sPendingLoad = false;
		sPendingClose = false;
		sHasPendingGenie = false;
		sPendingGenie.clear();
	}
	if (closeCartridge) {
		megad.plugged = false;
		megad.reset();
		post_status(B_TRANSLATE("Cartridge closed."));
	}
	if (saveState || loadState) {
		slot = stateSlot;
		if (saveState)
			md_save(megad);
		else
			md_load(megad);
	}
	if (applyGenie) {
		unsigned int errors = 0;
		unsigned int applied = 0;
		unsigned int reverted = 0;
		megad.patch(genieCode.c_str(), &errors, &applied, &reverted);
		if (errors != 0)
			post_status(B_TRANSLATE("Invalid code."));
		else if (reverted != 0)
			post_status(B_TRANSLATE("Reverted."));
		else if (applied != 0)
			post_status(B_TRANSLATE("Applied."));
		else
			post_status(B_TRANSLATE("Aborted."));
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
	pd_message(B_TRANSLATE("Cartridge: %s"), megad.romname);
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
	if (sWindow != NULL)
		sWindow->PostMessage(MSG_TOGGLE_FULLSCREEN);
}

void dgen_haiku_set_scale(int scale)
{
	sScale.store(scale);
	if (sWindow == NULL)
		return;
	BMessage message(MSG_SET_SCALE);
	message.AddInt32("scale", scale);
	sWindow->PostMessage(&message);
}

void dgen_haiku_open_rom(const char *path)
{
	if (path == NULL)
		return;
	std::lock_guard<std::mutex> lock(sPathLock);
	sPendingPath = path;
	sHasPendingPath = true;
}
