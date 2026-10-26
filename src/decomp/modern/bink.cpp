// bink.cpp - the Bink calls of the movie player for the 64-bit build (docs/future/modern-port.md, rung L3). Not
// original source; compiled only into MODERN_BUILD, and only does anything in a 64-bit exe (the 32-bit builds link
// the real binkw32.dll, which has no 64-bit version).
//
// The game uses eight Bink calls (vsdk/unnamed_movie_0x5172f0.cpp). Built with FFmpeg's headers (link-build with
// FFMPEG_DIR), they decode the movie with FFmpeg, whose libavcodec reads Bink 1 video and audio: the DLLs
// (avformat-63, avcodec-63, avutil-61, swscale-10, swresample-7) are loaded at run time from the exe's folder, so the
// game starts without them and then plays no movies. Video becomes RGB565 for BinkCopyToBuffer, paced by the clock in
// BinkWait; audio becomes 16-bit stereo through waveOut. Without FFmpeg's headers, BinkOpen fails and the game skips
// the movie, as it does without binkw32.
#if defined(MODERN_BUILD) && defined(_WIN64)
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "modern/modern.h"

// The game's view of the handle (unnamed_movie_0x5172f0.cpp's BINK): these four fields first.
struct bink_header {
    unsigned int Width, Height, Frames, FrameNum;
};

#if __has_include(<libavformat/avformat.h>)
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// FFmpeg's functions, loaded once from the DLLs beside the exe.
#define FF_FUNCS(X) \
    X(avformat, avformat_open_input) X(avformat, avformat_find_stream_info) X(avformat, av_find_best_stream) \
    X(avformat, av_read_frame) X(avformat, avformat_close_input) \
    X(avcodec, avcodec_find_decoder) X(avcodec, avcodec_alloc_context3) X(avcodec, avcodec_parameters_to_context) \
    X(avcodec, avcodec_open2) X(avcodec, avcodec_send_packet) X(avcodec, avcodec_receive_frame) \
    X(avcodec, avcodec_free_context) X(avcodec, av_packet_alloc) X(avcodec, av_packet_free) X(avcodec, av_packet_unref) \
    X(avutil, av_frame_alloc) X(avutil, av_frame_free) X(avutil, av_channel_layout_default) \
    X(swscale, sws_getContext) X(swscale, sws_scale) X(swscale, sws_freeContext) \
    X(swresample, swr_alloc_set_opts2) X(swresample, swr_init) X(swresample, swr_convert) X(swresample, swr_free) \
    X(swresample, swr_get_out_samples)
#define FF_DECLARE(lib, name) static decltype(&name) p_##name;
FF_FUNCS(FF_DECLARE)

static int ff_state;   // 0 not tried, 1 loaded, -1 missing

static bool ff_load()
{
    if (ff_state)
        return ff_state > 0;
    ff_state = -1;
    const char *dlls[] = { "avutil-61.dll", "swresample-7.dll", "swscale-10.dll", "avcodec-63.dll", "avformat-63.dll" };
    HMODULE avutil = NULL, swresample = NULL, swscale = NULL, avcodec = NULL, avformat = NULL;
    HMODULE *mods[] = { &avutil, &swresample, &swscale, &avcodec, &avformat };
    for (int i = 0; i < 5; i++) {
        char path[MAX_PATH];
        _snprintf(path, MAX_PATH, "%s\\%s", modern_dir(), dlls[i]);
        *mods[i] = LoadLibraryA(path);
        if (!*mods[i]) {
            modern_log("movies: %s not found; movies are skipped", dlls[i]);
            return false;
        }
    }
#define FF_LOAD(lib, name) \
    if (!(p_##name = (decltype(&name))GetProcAddress(lib, #name))) { modern_log("movies: %s missing", #name); return false; }
    FF_FUNCS(FF_LOAD)
    ff_state = 1;
    modern_log("movies: FFmpeg loaded");
    return true;
}

#define AUDIO_BUFFERS 64
struct bink_movie {
    bink_header h;
    AVFormatContext *fmt;
    AVCodecContext *vdec, *adec;
    int vstream, astream;
    AVPacket *pkt;
    AVFrame *frame, *aframe;
    SwsContext *sws;
    SwrContext *swr;
    bool have_frame;
    double fps;
    LARGE_INTEGER start, freq;
    HWAVEOUT wave;
    WAVEHDR hdr[AUDIO_BUFFERS];
    int next_hdr;
    int dropped;          // audio chunks dropped with every buffer busy
    double max_late;      // the latest a frame was shown after its time, in seconds
};

static void audio_reap(bink_movie *m)
{
    for (int i = 0; i < AUDIO_BUFFERS; i++) {
        WAVEHDR *w = &m->hdr[i];
        if (w->lpData && (w->dwFlags & WHDR_DONE)) {
            waveOutUnprepareHeader(m->wave, w, sizeof *w);
            free(w->lpData);
            memset(w, 0, sizeof *w);
        }
    }
}

// Decoded audio: converted to 16-bit stereo at the stream's rate and queued on waveOut.
static void audio_queue(bink_movie *m)
{
    if (!m->wave || !m->swr)
        return;
    audio_reap(m);
    int cap = p_swr_get_out_samples(m->swr, m->aframe->nb_samples);
    if (cap <= 0)
        return;
    WAVEHDR *w = &m->hdr[m->next_hdr];
    if (w->lpData) {   // all buffers still playing: this chunk is dropped
        m->dropped++;
        return;
    }
    unsigned char *out = (unsigned char *)malloc((size_t)cap * 4);
    if (!out)
        return;
    int n = p_swr_convert(m->swr, &out, cap, (const unsigned char **)m->aframe->extended_data, m->aframe->nb_samples);
    if (n <= 0) {
        free(out);
        return;
    }
    w->lpData = (LPSTR)out;
    w->dwBufferLength = (DWORD)n * 4;
    waveOutPrepareHeader(m->wave, w, sizeof *w);
    waveOutWrite(m->wave, w, sizeof *w);
    m->next_hdr = (m->next_hdr + 1) % AUDIO_BUFFERS;
}

static AVCodecContext *open_decoder(AVStream *st)
{
    const AVCodec *codec = p_avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec)
        return NULL;
    AVCodecContext *c = p_avcodec_alloc_context3(codec);
    if (!c || p_avcodec_parameters_to_context(c, st->codecpar) < 0 || p_avcodec_open2(c, codec, NULL) < 0) {
        p_avcodec_free_context(&c);
        return NULL;
    }
    return c;
}

extern "C" void *WINAPI BinkOpen(const char *name, unsigned int flags)
{
    (void)flags;
    if (!ff_load())
        return NULL;
    bink_movie *m = (bink_movie *)calloc(1, sizeof(bink_movie));
    if (!m || p_avformat_open_input(&m->fmt, name, NULL, NULL) < 0 || p_avformat_find_stream_info(m->fmt, NULL) < 0) {
        modern_log("movies: cannot open %s", name);
        if (m && m->fmt)
            p_avformat_close_input(&m->fmt);
        free(m);
        return NULL;
    }
    m->vstream = p_av_find_best_stream(m->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    m->astream = p_av_find_best_stream(m->fmt, AVMEDIA_TYPE_AUDIO, -1, m->vstream, NULL, 0);
    m->vdec = m->vstream >= 0 ? open_decoder(m->fmt->streams[m->vstream]) : NULL;
    if (!m->vdec) {
        modern_log("movies: no video decoder for %s", name);
        p_avformat_close_input(&m->fmt);
        free(m);
        return NULL;
    }
    AVStream *vs = m->fmt->streams[m->vstream];
    m->fps = vs->avg_frame_rate.num > 0 ? (double)vs->avg_frame_rate.num / vs->avg_frame_rate.den : 25.0;
    m->h.Width = (unsigned int)m->vdec->width;
    m->h.Height = (unsigned int)m->vdec->height;
    m->h.Frames = vs->nb_frames > 0 ? (unsigned int)vs->nb_frames
                                    : (unsigned int)(m->fmt->duration / (double)AV_TIME_BASE * m->fps + 0.5);
    m->h.FrameNum = 1;
    m->pkt = p_av_packet_alloc();
    m->frame = p_av_frame_alloc();
    m->aframe = p_av_frame_alloc();

    if (m->astream >= 0 && (m->adec = open_decoder(m->fmt->streams[m->astream])) != NULL) {
        AVChannelLayout stereo;
        p_av_channel_layout_default(&stereo, 2);
        int rate = m->adec->sample_rate;
        if (p_swr_alloc_set_opts2(&m->swr, &stereo, AV_SAMPLE_FMT_S16, rate, &m->adec->ch_layout, m->adec->sample_fmt,
                                  rate, 0, NULL) < 0 || p_swr_init(m->swr) < 0)
            p_swr_free(&m->swr);
        WAVEFORMATEX wf = { WAVE_FORMAT_PCM, 2, (DWORD)rate, (DWORD)rate * 4, 4, 16, 0 };
        if (m->swr && waveOutOpen(&m->wave, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
            m->wave = NULL;
    }
    QueryPerformanceFrequency(&m->freq);
    QueryPerformanceCounter(&m->start);
    modern_log("movies: %s, %ux%u, %u frames at %.2f fps, audio %s", name, m->h.Width, m->h.Height, m->h.Frames, m->fps,
               m->wave ? "on" : "off");
    return m;
}

// Decodes the next video frame, queueing the audio read on the way.
extern "C" int WINAPI BinkDoFrame(void *bink)
{
    bink_movie *m = (bink_movie *)bink;
    m->have_frame = false;
    for (;;) {
        if (p_avcodec_receive_frame(m->vdec, m->frame) == 0) {
            m->have_frame = true;
            return 0;
        }
        if (p_av_read_frame(m->fmt, m->pkt) < 0) {
            p_avcodec_send_packet(m->vdec, NULL);   // drain
            m->have_frame = p_avcodec_receive_frame(m->vdec, m->frame) == 0;
            return 0;
        }
        if (m->pkt->stream_index == m->vstream) {
            p_avcodec_send_packet(m->vdec, m->pkt);
        } else if (m->adec && m->pkt->stream_index == m->astream) {
            if (p_avcodec_send_packet(m->adec, m->pkt) == 0)
                while (p_avcodec_receive_frame(m->adec, m->aframe) == 0)
                    audio_queue(m);
        }
        p_av_packet_unref(m->pkt);
    }
}

extern "C" int WINAPI BinkCopyToBuffer(void *bink, void *dest, int destpitch, unsigned int destheight,
                                       unsigned int destx, unsigned int desty, unsigned int flags)
{
    (void)destheight;
    (void)flags;   // always BINKSURFACE565 | BINKCOPYALL in this game
    bink_movie *m = (bink_movie *)bink;
    if (!m->have_frame)
        return 1;   // nothing new: skipped
    m->sws = (SwsContext *)p_sws_getContext(m->frame->width, m->frame->height, (AVPixelFormat)m->frame->format,
                                            m->frame->width, m->frame->height, AV_PIX_FMT_RGB565LE, SWS_POINT, NULL,
                                            NULL, NULL);
    if (!m->sws)
        return 1;
    uint8_t *dst[4] = { (uint8_t *)dest + (size_t)desty * destpitch + (size_t)destx * 2, NULL, NULL, NULL };
    int dst_pitch[4] = { destpitch, 0, 0, 0 };
    p_sws_scale(m->sws, m->frame->data, m->frame->linesize, 0, m->frame->height, dst, dst_pitch);
    p_sws_freeContext(m->sws);
    m->sws = NULL;
    return 0;
}

extern "C" void WINAPI BinkNextFrame(void *bink)
{
    bink_movie *m = (bink_movie *)bink;
    if (m->h.FrameNum < m->h.Frames)
        m->h.FrameNum++;
}

// Nonzero while the current frame's time has not come yet (the player then polls again).
extern "C" int WINAPI BinkWait(void *bink)
{
    bink_movie *m = (bink_movie *)bink;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double t = (double)(now.QuadPart - m->start.QuadPart) / (double)m->freq.QuadPart;
    if (t < (m->h.FrameNum - 1) / m->fps) {
        Sleep(1);
        return 1;
    }
    if (t - (m->h.FrameNum - 1) / m->fps > m->max_late)
        m->max_late = t - (m->h.FrameNum - 1) / m->fps;
    return 0;
}

extern "C" void WINAPI BinkClose(void *bink)
{
    bink_movie *m = (bink_movie *)bink;
    if (!m)
        return;
    // Sync: the video time shown, the wall time, and the audio played so far (waveOut's position).
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    MMTIME pos = { TIME_SAMPLES };
    double audio = -1;
    if (m->wave && waveOutGetPosition(m->wave, &pos, sizeof pos) == MMSYSERR_NOERROR && pos.wType == TIME_SAMPLES)
        audio = (double)pos.u.sample / m->adec->sample_rate;
    modern_log("movies: closed at frame %u of %u: video %.2f s, wall %.2f s, audio %.2f s; latest frame %.0f ms late, "
               "%d audio chunks dropped", m->h.FrameNum, m->h.Frames, (m->h.FrameNum - 1) / m->fps,
               (double)(now.QuadPart - m->start.QuadPart) / (double)m->freq.QuadPart, audio, m->max_late * 1000, m->dropped);
    if (m->wave) {
        waveOutReset(m->wave);
        audio_reap(m);
        for (int i = 0; i < AUDIO_BUFFERS; i++)
            if (m->hdr[i].lpData) {
                waveOutUnprepareHeader(m->wave, &m->hdr[i], sizeof m->hdr[i]);
                free(m->hdr[i].lpData);
            }
        waveOutClose(m->wave);
    }
    p_swr_free(&m->swr);
    p_av_frame_free(&m->frame);
    p_av_frame_free(&m->aframe);
    p_av_packet_free(&m->pkt);
    p_avcodec_free_context(&m->vdec);
    p_avcodec_free_context(&m->adec);
    p_avformat_close_input(&m->fmt);
    free(m);
}

#else   // no FFmpeg headers at build time: no movies, as without binkw32

extern "C" void *WINAPI BinkOpen(const char *name, unsigned int flags) { (void)name; (void)flags; return NULL; }
extern "C" void WINAPI BinkClose(void *bink) { (void)bink; }
extern "C" int WINAPI BinkCopyToBuffer(void *bink, void *dest, int destpitch, unsigned int destheight,
                                       unsigned int destx, unsigned int desty, unsigned int flags)
{ (void)bink; (void)dest; (void)destpitch; (void)destheight; (void)destx; (void)desty; (void)flags; return 1; }
extern "C" void WINAPI BinkNextFrame(void *bink) { (void)bink; }
extern "C" int WINAPI BinkWait(void *bink) { (void)bink; return 0; }
extern "C" int WINAPI BinkDoFrame(void *bink) { (void)bink; return 0; }
#endif

// Bink's own DirectSound output: the audio above goes through waveOut instead.
extern "C" void *WINAPI BinkOpenDirectSound(unsigned int param) { (void)param; return NULL; }
extern "C" int WINAPI BinkSetSoundSystem(void *open, unsigned int param) { (void)open; (void)param; return 1; }
#endif
