// vsdk/snd/unnamed_sndstream_0x566560.h - streamed sounds (snd_stream_*), the MS-ADPCM converter (acm_*)
// and the RIFF wave reader (wav_*) (0x566560-0x56742b, inside the libjpeg run; source at
// src/decomp/vsdk/unnamed_sndstream_0x566560.cpp, decomp66; file name a placeholder). Not original source.
#ifndef VSDK_SND_UNNAMED_SNDSTREAM_0X566560_H
#define VSDK_SND_UNNAMED_SNDSTREAM_0X566560_H

#include "vsdk/snd/ds.h"

// snd_stream_slot::flags
#define SND_STREAM_USED    0x1
#define SND_STREAM_LOOPING 0x2
#define SND_STREAM_ENDED   0x4        // the file is read out
// One stream (0x1c bytes; snd_streams 0x2fbd9b0, 16 records). A multimedia timer services it every
// 250 ms under snd_stream_lock.
struct snd_stream_slot {
    int buffer;                       // +0x00 ds_buffers index, -1
    int channel;                      // +0x04 ds_channels index, -1
    unsigned int ring_size;           // +0x08 2 seconds of the PCM format
    unsigned int write_pos;           // +0x0c
    unsigned int timer;               // +0x10 timeSetEvent id, 0 none
    unsigned int silence;             // +0x14 silence bytes written after the end
    unsigned int flags;               // +0x18 SND_STREAM_*
};
LAYOUT_CHECK(snd_stream_slot, sizeof(snd_stream_slot) == 0x1c);
#define SND_MAX_STREAMS 16

#define SND_ACM_OUT_SIZE 0x2bf20      // 180000 bytes (name ours)
extern ubyte snd_acm_out_buf[SND_ACM_OUT_SIZE];   // 0x2f91a78
extern CRITICAL_SECTION snd_stream_lock;          // 0x2fbd998
extern snd_stream_slot snd_streams[SND_MAX_STREAMS];   // 0x2fbd9b0
extern HACMDRIVER acm_driver_handle;              // 0x2fbdb70
extern HMODULE acm_dll;                           // 0x2fbdb74 (name ours; freed at once)
extern HACMDRIVERID acm_driver_id;                // 0x2fbdb78
extern bool acm_initialized;                      // 0x2fbdb7c

// (delay ms, stream): a periodic timer calling snd_timer_proc.
MMRESULT snd_start_timer(int delay, int stream);  // 0x566560
void CALLBACK snd_timer_proc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2);   // 0x566580
void snd_stream_kill_timer(snd_stream_slot *s);   // 0x5665b0
unsigned int snd_stream_get_writable(int stream); // 0x5665d0 bytes from the write position to the play cursor
// (stream, bytes, &written): 0 / -1.
int  snd_stream_fill_acm(int stream, unsigned int bytes, unsigned int *written);    // 0x566630
int  snd_stream_fill_pcm(int stream, unsigned int bytes, unsigned int *written);    // 0x566820
int  snd_stream_fill_silence(int stream, unsigned int bytes);   // 0x566970
void snd_stream_stop(int stream);                 // 0x566ab0
int  snd_stream_rewind(int stream);               // 0x566b10
void snd_stream_service(int stream);              // 0x566b60
int  snd_stream_find_free();                      // 0x566c30
int  snd_stream_start(int buffer, int channel, bool looping);   // 0x566c60 the stream, -1
void snd_stream_init();                           // 0x566cf0
void snd_stream_close_all();                      // 0x566d30

BOOL CALLBACK acm_driver_enum_callback(HACMDRIVERID id, DWORD instance, DWORD support);   // 0x566d70
// (file format, PCM format filled in, &stream, bits): 0 / -1.
int  acm_stream_open(WAVEFORMATEX *src, WAVEFORMATEX *dst, HACMSTREAM *out, int bits);   // 0x566e20
int  acm_stream_close(HACMSTREAM s);              // 0x566eb0
// (stream, src, src bytes, dst, dst bytes, &dst used, &src used): 0 / -1.
int  acm_convert(HACMSTREAM s, void *src, DWORD src_len, void *dst, DWORD dst_len, DWORD *dst_used,
                 DWORD *src_used);                // 0x566ed0
int  acm_stream_size(HACMSTREAM s, DWORD dst_bytes);   // 0x566f80 the source bytes for that output
int  acm_stream_size_b(HACMSTREAM s, DWORD src_bytes); // 0x566fb0 the output bytes for that source
int  acm_init();                                  // 0x566fe0 the MS-ADPCM driver
void acm_close();                                 // 0x567080

// (path, offset, &hmmio, &file format, &PCM format, &riff chunk): 0 / -1 (both formats malloc'd).
int  wav_open(const char *path, int offset, HMMIO *out, WAVEFORMATEX **fmt, WAVEFORMATEX **pcm,
              MMCKINFO *riff);                    // 0x5670a0
int  wav_descend_data(HMMIO *h, MMCKINFO *data, MMCKINFO *riff);   // 0x567300
int  wav_read(HMMIO h, UINT bytes, BYTE *dest, MMCKINFO *ck, UINT *read);   // 0x567350
void wav_close(HMMIO *h);                         // 0x567410

#endif
