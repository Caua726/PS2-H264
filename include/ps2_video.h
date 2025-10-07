#ifndef PS2_VIDEO_H
#define PS2_VIDEO_H

#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GSGLOBAL *gs_global;
    GSTEXTURE frame_textures[2];
    u32 tex_width;
    u32 tex_height;
    u32 frame_width;
    u32 frame_height;
    int current_upload;
    int current_display;
    int ready;
} Ps2VideoContext;

int ps2_video_bootstrap(Ps2VideoContext *ctx);
int ps2_video_prepare(Ps2VideoContext *ctx, u32 frame_width, u32 frame_height);
void ps2_video_upload_frame(Ps2VideoContext *ctx, const u32 *rgba);
void ps2_video_present(Ps2VideoContext *ctx);
void ps2_video_shutdown(Ps2VideoContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PS2_VIDEO_H */
