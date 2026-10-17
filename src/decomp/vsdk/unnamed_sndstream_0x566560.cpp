// unnamed_sndstream_0x566560.cpp - Sum.exe v1.40, code 0x566560-0x56742b: streamed sounds (music, cutscene
// soundtracks, console sounds) refilled by a multimedia timer, the MS-ADPCM converter (msacm32.dll) and the
// RIFF wave reader. The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): between two runs of the libjpeg objects (jerror.c's reset_error_mgr
// 0x566550 before, jmemmgr.c's jinit_memory_mgr 0x567430 after). .data 0x5af494-0x5af4b3 ("????" used
// twice, one literal; "MS-ADPCM"; "msacm32.dll") after jerror.c's "%s\n"; .bss 0x2f91a78-0x2fbdb7f (the
// ACM output buffer, the lock, the streams, then the ACM driver state) after the vfx mesh unit's. The
// stream, ACM and wave parts could be three files (FreeSpace 2 has them apart); nothing in the layout
// separates them, so they are one placeholder here.

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <string.h>
#include <stdlib.h>
#include "vsdk/snd/unnamed_sndstream_0x566560.h"

// ---- .bss -------------------------------------------------------------------------------------------
ubyte snd_acm_out_buf[SND_ACM_OUT_SIZE];          // 0x2f91a78
CRITICAL_SECTION snd_stream_lock;                 // 0x2fbd998
snd_stream_slot snd_streams[SND_MAX_STREAMS];     // 0x2fbd9b0
HACMDRIVER acm_driver_handle;                     // 0x2fbdb70
HMODULE acm_dll;                                  // 0x2fbdb74
HACMDRIVERID acm_driver_id;                       // 0x2fbdb78
bool acm_initialized;                             // 0x2fbdb7c

// 0x566560  Confidence: exact.
MMRESULT snd_start_timer(int delay, int stream)
{
    return timeSetEvent(delay, delay, snd_timer_proc, stream, TIME_PERIODIC);
}

// 0x566580  Confidence: exact.
void CALLBACK snd_timer_proc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
    EnterCriticalSection(&snd_stream_lock);
    snd_stream_service(user);
    LeaveCriticalSection(&snd_stream_lock);
}

// 0x5665b0  Confidence: exact.
void snd_stream_kill_timer(snd_stream_slot *s)
{
    if (s->timer != 0) {
        timeKillEvent(s->timer);
        s->timer = 0;
    }
}

// 0x5665d0  Confidence: exact.
unsigned int snd_stream_get_writable(int stream)
{
    snd_stream_slot *s = &snd_streams[stream];
    DWORD play;
    DWORD write;

    if (ds_channels[s->channel].pdsb->GetCurrentPosition(&play, &write) < 0)
        return 0;
    unsigned int n;
    if (s->write_pos <= play)
        n = play - s->write_pos;
    else
        n = s->ring_size - s->write_pos + play;
    return n;
}

// 0x566630  Confidence: likely (the shared exit unlocks even after a failed Lock). The converted bytes are
// copied into the two locked parts of the ring.
int snd_stream_fill_acm(int stream, unsigned int bytes, unsigned int *written)
{
    snd_stream_slot *s = &snd_streams[stream];
    ds_buffer *b = &ds_buffers[s->buffer];
    IDirectSoundBuffer *pdsb = ds_channels[s->channel].pdsb;
    void *p1 = NULL;
    void *p2 = NULL;
    DWORD n1 = 0;
    DWORD n2 = 0;
    UINT got = 0;
    DWORD dst_used;
    DWORD src_used;
    int src_size;
    int rc = 0;

    *written = 0;
    if (pdsb->Lock(s->write_pos, bytes, &p1, &n1, &p2, &n2, 0) < 0)
        goto fail;
    src_size = acm_stream_size(b->acm_stream, n1 + n2);
    if (src_size <= 0)
        goto fail;
    if (wav_read(b->hmmio, src_size, ds_stream_src_buffer, &b->data_ck, &got) != 0)
        goto fail;
    if (got > 0) {
        if (acm_convert(b->acm_stream, ds_stream_src_buffer, got, snd_acm_out_buf, sizeof(snd_acm_out_buf),
                        &dst_used, &src_used) == -1)
            goto fail;
        if (src_used < got)
            mmioSeek(b->hmmio, src_used - got, SEEK_CUR);
        if (dst_used > n1) {
            memcpy(p1, snd_acm_out_buf, n1);
            memcpy(p2, snd_acm_out_buf + n1, dst_used - n1);
        } else {
            memcpy(p1, snd_acm_out_buf, dst_used);
        }
        *written += dst_used;
    } else {
        s->flags |= SND_STREAM_ENDED;
    }
    s->write_pos = (s->write_pos + *written) % s->ring_size;
    goto done;
fail:
    rc = -1;
done:
    pdsb->Unlock(p1, n1, p2, n2);
    return rc;
}

// 0x566820  Confidence: likely (as snd_stream_fill_acm).
int snd_stream_fill_pcm(int stream, unsigned int bytes, unsigned int *written)
{
    snd_stream_slot *s = &snd_streams[stream];
    ds_buffer *b = &ds_buffers[s->buffer];
    IDirectSoundBuffer *pdsb = ds_channels[s->channel].pdsb;
    void *p1 = NULL;
    void *p2 = NULL;
    DWORD n1 = 0;
    DWORD n2 = 0;
    UINT got = 0;
    int rc = 0;

    *written = 0;
    if (pdsb->Lock(s->write_pos, bytes, &p1, &n1, &p2, &n2, 0) < 0)
        goto fail;
    if (p1 != NULL && wav_read(b->hmmio, n1, (BYTE *)p1, &b->data_ck, &got) != 0)
        goto fail;
    *written += got;
    got = 0;
    if (p2 != NULL && wav_read(b->hmmio, n2, (BYTE *)p2, &b->data_ck, &got) != 0)
        goto fail;
    *written += got;
    s->write_pos = (s->write_pos + *written) % s->ring_size;
    if (*written < bytes)
        s->flags |= SND_STREAM_ENDED;
    goto done;
fail:
    rc = -1;
done:
    pdsb->Unlock(p1, n1, p2, n2);
    return rc;
}

// 0x566970  Confidence: likely (the fill byte lives in the stream argument's slot). 0x80 for 8-bit,
// 0 for 16-bit.
int snd_stream_fill_silence(int stream, unsigned int bytes)
{
    snd_stream_slot *s = &snd_streams[stream];
    void *p1 = NULL;
    void *p2 = NULL;
    DWORD n1 = 0;
    DWORD n2 = 0;
    DWORD extra;
    ubyte fill = 0x80;

    if (ds_buffers[s->buffer].pcm_format->wBitsPerSample == 16)
        fill = 0;
    if (ds_channels[s->channel].pdsb->Lock(s->write_pos, bytes, &p1, &n1, &p2, &n2, 0) < 0)
        return -1;
    extra = 0;
    memset(p1, fill, n1);
    if (p2 != NULL) {
        memset(p2, fill, n2);
        extra = n2;
    }
    s->write_pos = (s->write_pos + n1 + extra) % s->ring_size;
    ds_channels[s->channel].pdsb->Unlock(p1, n1, p2, n2);
    return 0;
}

// 0x566ab0  Confidence: exact.
void snd_stream_stop(int stream)
{
    snd_stream_slot *s = &snd_streams[stream];

    if (s->channel >= 0) {
        if (ds_channels[s->channel].pdsb != NULL)
            ds_channels[s->channel].pdsb->Stop();
        s->channel = -1;
    }
    snd_stream_kill_timer(s);
    s->flags = 0;
    s->buffer = -1;
}

// 0x566b10  Confidence: exact.
int snd_stream_rewind(int stream)
{
    ds_buffer *b = &ds_buffers[snd_streams[stream].buffer];

    if (wav_descend_data(&b->hmmio, &b->data_ck, &b->riff_ck) != 0)
        return -1;
    return 0;
}

// 0x566b60  Confidence: exact. At the end of the file the rest of the window is filled with silence,
// also for a looping stream after its rewind (a gap at every loop; NOTES.md); a one-shot stream stops
// once four ring sizes of silence are written.
void snd_stream_service(int stream)
{
    snd_stream_slot *s = &snd_streams[stream];
    unsigned int n;
    unsigned int written;

    if (!(s->flags & SND_STREAM_USED))
        return;
    n = snd_stream_get_writable(stream);
    if (n <= 0)
        return;
    if (ds_buffers[s->buffer].format->wFormatTag == WAVE_FORMAT_ADPCM)
        snd_stream_fill_acm(stream, n, &written);
    else
        snd_stream_fill_pcm(stream, n, &written);
    if (!(s->flags & SND_STREAM_ENDED))
        return;
    n -= written;
    s->flags &= ~SND_STREAM_ENDED;
    if (s->flags & SND_STREAM_LOOPING)
        snd_stream_rewind(stream);
    s->silence += n;
    if (snd_stream_fill_silence(stream, n) != 0)
        return;
    if (s->flags & SND_STREAM_LOOPING)
        return;
    if (s->silence >= s->ring_size * 4) {
        s->silence = 0;
        snd_stream_stop(stream);
    }
}

// 0x566c30  Confidence: exact.
int snd_stream_find_free()
{
    int i;

    for (i = 0; i < SND_MAX_STREAMS; i++) {
        if (!(snd_streams[i].flags & SND_STREAM_USED))
            break;
    }
    if (i >= SND_MAX_STREAMS)
        return -1;
    return i;
}

// 0x566c60  Confidence: exact. The silence count of the record is not reset.
int snd_stream_start(int buffer, int channel, bool looping)
{
    snd_stream_slot *s;
    int i;

    i = snd_stream_find_free();
    if (i < 0)
        return -1;
    s = &snd_streams[i];
    s->flags = SND_STREAM_USED;
    s->buffer = buffer;
    s->channel = channel;
    s->write_pos = 0;
    s->ring_size = ds_buffers[buffer].pcm_format->nAvgBytesPerSec * 2;
    if (looping)
        s->flags |= SND_STREAM_LOOPING;
    s->timer = snd_start_timer(250, i);
    if (s->timer == 0) {
        s->flags = 0;
        return -1;
    }
    return i;
}

// 0x566cf0  Confidence: exact.
void snd_stream_init()
{
    int i;

    InitializeCriticalSection(&snd_stream_lock);
    timeBeginPeriod(1);
    for (i = 0; i < SND_MAX_STREAMS; i++) {
        snd_streams[i].timer = 0;
        snd_streams[i].silence = 0;
        snd_streams[i].write_pos = 0;
        snd_streams[i].channel = -1;
        snd_streams[i].buffer = -1;
    }
}

// 0x566d30  Confidence: exact.
void snd_stream_close_all()
{
    int i;

    for (i = 0; i < SND_MAX_STREAMS; i++) {
        if (snd_streams[i].flags & SND_STREAM_USED)
            snd_stream_stop(i);
    }
    timeEndPeriod(1);
    DeleteCriticalSection(&snd_stream_lock);
}

// 0x566d70  Confidence: exact (the FALSE of the found case is _stricmp's 0). Picks the "MS-ADPCM" driver
// (none when it is disabled) and stops the enumeration there.
BOOL CALLBACK acm_driver_enum_callback(HACMDRIVERID id, DWORD instance, DWORD support)
{
    ACMDRIVERDETAILS dd;
    DWORD priority;

    dd.cbStruct = sizeof(dd);
    if (acmDriverDetails(id, &dd, 0) != 0) {
        lstrcpy(dd.szShortName, "????");
        lstrcpy(dd.szLongName, "????");
    }
    priority = (DWORD)-1;
    acmMetrics((HACMOBJ)id, ACM_METRIC_DRIVER_PRIORITY, &priority);
    if (_stricmp("MS-ADPCM", dd.szShortName) == 0) {
        acm_driver_id = (support & ACMDRIVERDETAILS_SUPPORTF_DISABLED) ? NULL : id;
        return FALSE;
    }
    return TRUE;
}

// 0x566e20  Confidence: exact (the order of the format stores).
int acm_stream_open(WAVEFORMATEX *src, WAVEFORMATEX *dst, HACMSTREAM *out, int bits)
{
    if (!acm_initialized && acm_init() != 0)
        return -1;
    DWORD rate = src->nSamplesPerSec;
    dst->wBitsPerSample = (WORD)bits;
    dst->nChannels = src->nChannels;
    dst->nBlockAlign = (WORD)((dst->nChannels * (WORD)bits) / 8);
    dst->nAvgBytesPerSec = dst->nBlockAlign * rate;
    dst->wFormatTag = WAVE_FORMAT_PCM;
    dst->nSamplesPerSec = rate;
    dst->cbSize = 0;
    return acmStreamOpen(out, acm_driver_handle, src, dst, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME) != 0 ? -1 : 0;
}

// 0x566eb0  Confidence: exact.
int acm_stream_close(HACMSTREAM s)
{
    return acmStreamClose(s, 0) != 0 ? -1 : 0;
}

// 0x566ed0  Confidence: likely (a failed convert leaves the header prepared).
int acm_convert(HACMSTREAM s, void *src, DWORD src_len, void *dst, DWORD dst_len, DWORD *dst_used,
                DWORD *src_used)
{
    ACMSTREAMHEADER h;

    if (!acm_initialized)
        return -1;
    memset(&h, 0, sizeof(h));
    h.cbSrcLength = src_len;
    h.pbSrc = (LPBYTE)src;
    h.cbStruct = sizeof(h);
    h.pbDst = (LPBYTE)dst;
    h.cbDstLength = dst_len;
    if (acmStreamPrepareHeader(s, &h, 0) != 0)
        return -1;
    if (acmStreamConvert(s, &h, 0) != 0)
        return -1;
    *dst_used = h.cbDstLengthUsed;
    *src_used = h.cbSrcLengthUsed;
    return acmStreamUnprepareHeader(s, &h, 0) != 0 ? -1 : 0;
}

// 0x566f80  Confidence: exact.
int acm_stream_size(HACMSTREAM s, DWORD dst_bytes)
{
    DWORD n;

    if (acmStreamSize(s, dst_bytes, &n, ACM_STREAMSIZEF_DESTINATION) != 0)
        return -1;
    return n;
}

// 0x566fb0  Confidence: exact.
int acm_stream_size_b(HACMSTREAM s, DWORD src_bytes)
{
    DWORD n;

    if (acmStreamSize(s, src_bytes, &n, ACM_STREAMSIZEF_SOURCE) != 0)
        return -1;
    return n;
}

// 0x566fe0  Confidence: exact. ACM 3.50 or later.
int acm_init()
{
    if (acm_initialized == true)
        return 0;
    acm_dll = LoadLibrary("msacm32.dll");
    if (acm_dll == NULL)
        return -1;
    FreeLibrary(acm_dll);
    if ((int)acmGetVersion() < 0x3320000)
        return -1;
    if (acmDriverEnum(acm_driver_enum_callback, 0, ACM_DRIVERENUMF_DISABLED) != 0)
        return -1;
    if (acm_driver_id == NULL)
        return -1;
    if (acmDriverOpen(&acm_driver_handle, acm_driver_id, 0) != 0)
        return -1;
    if (acmDriverPriority(acm_driver_id, 0, 0) != 0)
        return -1;
    acm_initialized = true;
    return 0;
}

// 0x567080  Confidence: exact.
void acm_close()
{
    if (acm_initialized) {
        acmDriverClose(acm_driver_handle, 0);
        acm_initialized = false;
    }
}

// 0x5670a0  Confidence: likely (the error exits). The file's format goes to *fmt, the PCM format the
// buffer plays to *pcm (16-bit for MS-ADPCM).
int wav_open(const char *path, int offset, HMMIO *out, WAVEFORMATEX **fmt, WAVEFORMATEX **pcm, MMCKINFO *riff)
{
    HMMIO h;
    MMCKINFO ck;
    PCMWAVEFORMAT pcmwf;
    WORD extra;
    int rc = 0;

    *pcm = NULL;
    h = mmioOpen((LPSTR)path, NULL, MMIO_ALLOCBUF);
    if (h == NULL)
        goto fail;
    if (offset != 0)
        mmioSeek(h, offset, SEEK_SET);
    if (mmioDescend(h, riff, NULL, 0) != 0)
        goto fail;
    if (riff->ckid != FOURCC_RIFF || riff->fccType != mmioFOURCC('W', 'A', 'V', 'E'))
        goto fail;
    ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(h, &ck, riff, MMIO_FINDCHUNK) != 0)
        goto fail;
    if (ck.cksize < sizeof(PCMWAVEFORMAT))
        goto fail;
    if (mmioRead(h, (HPSTR)&pcmwf, sizeof(pcmwf)) != sizeof(pcmwf))
        goto fail;
    if (pcmwf.wf.wFormatTag == WAVE_FORMAT_PCM) {
        extra = 0;
    } else if (mmioRead(h, (HPSTR)&extra, sizeof(extra)) != sizeof(extra)) {
        goto fail;
    }
    *pcm = (WAVEFORMATEX *)malloc(sizeof(WAVEFORMATEX) + extra);
    if (*pcm == NULL)
        goto fail;
    *fmt = (WAVEFORMATEX *)malloc(sizeof(WAVEFORMATEX) + extra);
    if (*fmt == NULL)
        goto fail;
    memcpy(*fmt, &pcmwf, sizeof(pcmwf));
    (*fmt)->cbSize = extra;
    if (extra != 0 && mmioRead(h, (HPSTR)((char *)*fmt + sizeof(WAVEFORMATEX)), extra) != extra)
        goto fail;
    switch ((*fmt)->wFormatTag) {
    case WAVE_FORMAT_PCM:
        (*pcm)->wBitsPerSample = (*fmt)->wBitsPerSample;
        break;
    case WAVE_FORMAT_ADPCM:
        (*pcm)->wBitsPerSample = 16;
        break;
    default:
        goto fail;
    }
    (*pcm)->wFormatTag = WAVE_FORMAT_PCM;
    (*pcm)->nChannels = (*fmt)->nChannels;
    (*pcm)->nSamplesPerSec = (*fmt)->nSamplesPerSec;
    (*pcm)->cbSize = 0;
    (*pcm)->nBlockAlign = (WORD)(((*pcm)->wBitsPerSample * (*pcm)->nChannels) / 8);
    (*pcm)->nAvgBytesPerSec = (*pcm)->nBlockAlign * (*pcm)->nSamplesPerSec;
    if (mmioAscend(h, &ck, 0) != 0)
        goto fail;
    goto done;
fail:
    rc = -1;
    if (*pcm != NULL) {
        free(*pcm);
        *pcm = NULL;
    }
    if (*fmt != NULL) {
        free(*fmt);
        *fmt = NULL;
    }
    if (h != NULL) {
        mmioClose(h, 0);
        h = NULL;
    }
done:
    *out = h;
    return rc;

}

// 0x567300  Confidence: exact.
int wav_descend_data(HMMIO *h, MMCKINFO *data, MMCKINFO *riff)
{
    if (mmioSeek(*h, riff->dwDataOffset + 4, SEEK_SET) == -1)
        return -1;
    data->ckid = mmioFOURCC('d', 'a', 't', 'a');
    return mmioDescend(*h, data, riff, MMIO_FINDCHUNK) != 0 ? -1 : 0;
}

// 0x567350  Confidence: exact.
int wav_read(HMMIO h, UINT bytes, BYTE *dest, MMCKINFO *ck, UINT *read)
{
    MMIOINFO info;
    UINT i;
    int err = 0;

    if (mmioGetInfo(h, &info, 0) != 0)
        goto fail;
    if (bytes > ck->cksize)
        bytes = ck->cksize;
    ck->cksize -= bytes;
    for (i = 0; i < bytes; i++) {
        if (info.pchNext == info.pchEndRead) {
            if (mmioAdvance(h, &info, MMIO_READ) != 0)
                goto fail;
            if (info.pchNext == info.pchEndRead)
                goto fail;
        }
        dest[i] = *info.pchNext++;
    }
    if (mmioSetInfo(h, &info, 0) != 0)
        goto fail;
    *read = i;
    goto done;
fail:
    err = -1;
    *read = 0;
done:
    return err;

}

// 0x567410  Confidence: exact.
void wav_close(HMMIO *h)
{
    if (*h != NULL) {
        mmioClose(*h, 0);
        *h = NULL;
    }
}
