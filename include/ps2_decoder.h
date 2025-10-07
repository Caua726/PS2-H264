#ifndef PS2_DECODER_H
#define PS2_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <tamtypes.h>

#include "third_party/h264bsd/src/h264bsd_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    storage_t *storage;
    uint32_t width;
    uint32_t height;
    uint32_t cropping_flag;
    uint32_t crop_left;
    uint32_t crop_top;
    uint32_t crop_width;
    uint32_t crop_height;
    uint32_t next_pic_id;
} Ps2Decoder;

typedef struct {
    u32 pic_id;
    u32 is_idr;
    u32 num_err_mbs;
    u32 width;
    u32 height;
    u32 *pixels; /* RGBA (0xAABBGGRR) */
} Ps2DecodedFrame;

typedef enum {
    PS2_DECODER_OK = 0,
    PS2_DECODER_ERROR = -1
} Ps2DecoderResult;

int ps2_decoder_init(Ps2Decoder *decoder, int no_output_reordering);
void ps2_decoder_shutdown(Ps2Decoder *decoder);

int ps2_decoder_decode(Ps2Decoder *decoder, uint8_t *bitstream, uint32_t length,
                       uint32_t *out_consumed);

int ps2_decoder_has_dimensions(const Ps2Decoder *decoder);

int ps2_decoder_get_frame(Ps2Decoder *decoder, Ps2DecodedFrame *out_frame);

#ifdef __cplusplus
}
#endif

#endif /* PS2_DECODER_H */
