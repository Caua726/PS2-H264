#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <debug.h>
#include <stdbool.h>
#include <string.h>

#include "ps2_decoder.h"
#include "ps2_video.h"
#include "raw_video.h"

int main(void)
{
    init_scr();
    scr_printf("DecoderH264 PS2 player starting...\n");

    SifInitRpc(0);
    scr_printf("SifInitRpc done\n");

    Ps2Decoder decoder;
    if (ps2_decoder_init(&decoder, 0) != PS2_DECODER_OK) {
        scr_printf("Decoder init failed\n");
        SleepThread();
        return 0;
    }

    Ps2VideoContext video;
    if (ps2_video_bootstrap(&video) != 0) {
        scr_printf("GS bootstrap failed\n");
        ps2_decoder_shutdown(&decoder);
        SleepThread();
        return 0;
    }

    scr_printf("Streaming embedded bitstream (%u bytes)\n", g_video_size);

    const unsigned char *cursor = g_video_data;
    size_t remaining = g_video_size;
    bool headers_ready = false;
    int idle_loops = 0;
    unsigned long frame_count = 0;
    int last_status = -1;

    while (remaining > 0) {
        uint32_t consumed = 0;
        int status = ps2_decoder_decode(&decoder,
                                        (uint8_t *)cursor,
                                        (uint32_t)remaining,
                                        &consumed);
        if (status == H264BSD_ERROR) {
            scr_printf("Decode error\n");
            break;
        }
        if (status != last_status) {
            scr_printf("Decoder status=%d consumed=%lu\n", status,
                       (unsigned long)consumed);
            last_status = status;
        }

        if (!headers_ready && ps2_decoder_has_dimensions(&decoder)) {
            if (ps2_video_prepare(&video, decoder.width, decoder.height) != 0) {
                scr_printf("Failed to prepare GS texture\n");
                break;
            }
            scr_printf("Video resolution: %lux%lu\n",
                       (unsigned long)decoder.width,
                       (unsigned long)decoder.height);
            headers_ready = true;
        }

        Ps2DecodedFrame frame;
        while (ps2_decoder_get_frame(&decoder, &frame)) {
            if (!headers_ready)
                continue;
            ps2_video_upload_frame(&video, frame.pixels);
            ps2_video_present(&video);
            frame_count++;
            if ((frame_count & 0x1F) == 1) {
                scr_printf("Frame %lu (id=%lu err=%lu)\n",
                           frame_count,
                           (unsigned long)frame.pic_id,
                           (unsigned long)frame.num_err_mbs);
            }
        }

        if (consumed == 0) {
            if (remaining == 0)
                break;

            idle_loops++;
            if (idle_loops > 64) {
                scr_printf("Decoder stalled (status=%d)\n", status);
                break;
            }
            continue;
        }

        if (consumed > remaining) {
            scr_printf("Decoder consumed beyond buffer (consumed=%lu, remaining=%lu)\n",
                       (unsigned long)consumed, (unsigned long)remaining);
            break;
        }

        cursor += consumed;
        remaining -= consumed;
        idle_loops = 0;
    }

    Ps2DecodedFrame frame;
    while (ps2_decoder_get_frame(&decoder, &frame)) {
        if (headers_ready) {
            ps2_video_upload_frame(&video, frame.pixels);
            ps2_video_present(&video);
        }
    }

    scr_printf("Playback finished.\n");

    ps2_video_shutdown(&video);
    ps2_decoder_shutdown(&decoder);

    SleepThread();
    return 0;
}
