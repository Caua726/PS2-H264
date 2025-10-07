#include "ps2_decoder.h"

#include <malloc.h>
#include <string.h>

#include "third_party/h264bsd/src/h264bsd_cfg.h"
#include "third_party/h264bsd/src/h264bsd_util.h"

static void ps2_decoder_clear_dimensions(Ps2Decoder *decoder) {
    decoder->width = 0;
    decoder->height = 0;
    decoder->cropping_flag = 0;
    decoder->crop_left = 0;
    decoder->crop_top = 0;
    decoder->crop_width = 0;
    decoder->crop_height = 0;
}

int ps2_decoder_init(Ps2Decoder *decoder, int no_output_reordering) {
    if (!decoder)
        return PS2_DECODER_ERROR;

    memset(decoder, 0, sizeof(*decoder));
    decoder->storage = h264bsdAlloc();
    if (!decoder->storage)
        return PS2_DECODER_ERROR;

    ps2_decoder_clear_dimensions(decoder);
    decoder->next_pic_id = 0;

    u32 status = h264bsdInit(decoder->storage,
                             no_output_reordering ? HANTRO_TRUE : HANTRO_FALSE);
    if (status != HANTRO_OK) {
        h264bsdFree(decoder->storage);
        decoder->storage = NULL;
        return PS2_DECODER_ERROR;
    }

    return PS2_DECODER_OK;
}

void ps2_decoder_shutdown(Ps2Decoder *decoder) {
    if (!decoder || !decoder->storage)
        return;

    h264bsdShutdown(decoder->storage);
    h264bsdFree(decoder->storage);
    decoder->storage = NULL;
    ps2_decoder_clear_dimensions(decoder);
    decoder->next_pic_id = 0;
}

static void ps2_decoder_update_dimensions(Ps2Decoder *decoder) {
    if (!decoder || !decoder->storage)
        return;

    u32 cropping_flag = 0;
    u32 left = 0, width = 0, top = 0, height = 0;

    h264bsdCroppingParams(decoder->storage, &cropping_flag, &left, &width, &top,
                          &height);

    if (!cropping_flag) {
        width = h264bsdPicWidth(decoder->storage) * 16;
        height = h264bsdPicHeight(decoder->storage) * 16;
        left = 0;
        top = 0;
    }

    decoder->cropping_flag = cropping_flag;
    decoder->crop_left = left;
    decoder->crop_top = top;
    decoder->crop_width = width;
    decoder->crop_height = height;
    decoder->width = width;
    decoder->height = height;
}

int ps2_decoder_decode(Ps2Decoder *decoder, uint8_t *bitstream, uint32_t length,
                       uint32_t *out_consumed) {
    if (!decoder || !decoder->storage || !bitstream)
        return H264BSD_ERROR;

    u32 consumed = 0;
    u32 result = h264bsdDecode(decoder->storage, bitstream, length,
                               decoder->next_pic_id, &consumed);

    if (out_consumed)
        *out_consumed = consumed;

    switch (result) {
        case H264BSD_HDRS_RDY:
            ps2_decoder_update_dimensions(decoder);
            break;
        case H264BSD_PIC_RDY:
            decoder->next_pic_id++;
            break;
        case H264BSD_RDY:
            break;
        case H264BSD_ERROR:
        case H264BSD_PARAM_SET_ERROR:
        case H264BSD_MEMALLOC_ERROR:
            return H264BSD_ERROR;
        default:
            break;
    }

    return (int)result;
}

int ps2_decoder_has_dimensions(const Ps2Decoder *decoder) {
    if (!decoder)
        return 0;
    return decoder->width != 0 && decoder->height != 0;
}

int ps2_decoder_get_frame(Ps2Decoder *decoder, Ps2DecodedFrame *out_frame) {
    if (!decoder || !decoder->storage || !out_frame)
        return 0;

    u32 pic_id = 0;
    u32 is_idr = 0;
    u32 num_err_mbs = 0;

    u32 *pixels = h264bsdNextOutputPictureRGBA(decoder->storage, &pic_id, &is_idr,
                                               &num_err_mbs);
    if (!pixels)
        return 0;

    out_frame->pic_id = pic_id;
    out_frame->is_idr = is_idr;
    out_frame->num_err_mbs = num_err_mbs;
    out_frame->width = decoder->width;
    out_frame->height = decoder->height;
    out_frame->pixels = pixels;
    return 1;
}
