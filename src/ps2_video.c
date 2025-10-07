#include "ps2_video.h"

#include <malloc.h>
#include <string.h>

#include <gsToolkit.h>

#define ALIGN_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

static u32 next_power_of_two(u32 value) {
    u32 power = 1;
    while (power < value) {
        power <<= 1;
    }
    return power;
}

int ps2_video_bootstrap(Ps2VideoContext *ctx) {
    if (!ctx)
        return -1;

    memset(ctx, 0, sizeof(*ctx));
    ctx->current_upload = 0;
    ctx->current_display = -1;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    ctx->gs_global = gsKit_init_global();
    if (!ctx->gs_global)
        return -1;

    ctx->gs_global->Mode = GS_MODE_NTSC;
    ctx->gs_global->Width = 640;
    ctx->gs_global->Height = 448;
    ctx->gs_global->Interlace = GS_INTERLACED;
    ctx->gs_global->Field = GS_FIELD;
    ctx->gs_global->PSM = GS_PSM_CT32;
    ctx->gs_global->PSMZ = GS_PSMZ_16;
    ctx->gs_global->DoubleBuffering = GS_SETTING_ON;
    ctx->gs_global->ZBuffering = GS_SETTING_OFF;

    gsKit_init_screen(ctx->gs_global);
    gsKit_mode_switch(ctx->gs_global, GS_PERSISTENT);

    gsKit_clear(ctx->gs_global, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
    gsKit_queue_exec(ctx->gs_global);
    gsKit_sync_flip(ctx->gs_global);

    return 0;
}

static void ps2_video_release_texture(Ps2VideoContext *ctx) {
    for (int i = 0; i < 2; ++i) {
        if (ctx->frame_textures[i].Mem) {
            if (ctx->gs_global)
                gsKit_TexManager_free(ctx->gs_global, &ctx->frame_textures[i]);
            free(ctx->frame_textures[i].Mem);
            ctx->frame_textures[i].Mem = NULL;
        }
        ctx->frame_textures[i].Vram = 0;
    }
    ctx->ready = 0;
    ctx->current_upload = 0;
    ctx->current_display = -1;
}

int ps2_video_prepare(Ps2VideoContext *ctx, u32 frame_width, u32 frame_height) {
    if (!ctx || !ctx->gs_global)
        return -1;

    u32 tex_w = next_power_of_two(frame_width);
    u32 tex_h = next_power_of_two(frame_height);
    tex_w = tex_w < 64 ? 64 : tex_w;
    tex_h = tex_h < 64 ? 64 : tex_h;

    if (ctx->ready && ctx->tex_width == tex_w && ctx->tex_height == tex_h) {
        ctx->frame_width = frame_width;
        ctx->frame_height = frame_height;
        return 0;
    }

    ps2_video_release_texture(ctx);

    size_t tex_size = gsKit_texture_size(tex_w, tex_h, GS_PSM_CT32);
    for (int i = 0; i < 2; ++i) {
        void *buffer = memalign(128, tex_size);
        if (!buffer) {
            ps2_video_release_texture(ctx);
            return -1;
        }

        memset(&ctx->frame_textures[i], 0, sizeof(GSTEXTURE));
        ctx->frame_textures[i].Width = tex_w;
        ctx->frame_textures[i].Height = tex_h;
        ctx->frame_textures[i].PSM = GS_PSM_CT32;
        ctx->frame_textures[i].ClutPSM = GS_PSM_CT32;
        ctx->frame_textures[i].Filter = GS_FILTER_NEAREST;
        ctx->frame_textures[i].Clut = NULL;
        ctx->frame_textures[i].VramClut = 0;
        ctx->frame_textures[i].ClutStorageMode = GS_CLUT_STORAGE_CSM1;
        ctx->frame_textures[i].Delayed = GS_SETTING_OFF;
        ctx->frame_textures[i].Mem = buffer;
        ctx->frame_textures[i].TBW = tex_w / 64;
        if (ctx->frame_textures[i].TBW == 0)
            ctx->frame_textures[i].TBW = 1;
        gsKit_setup_tbw(&ctx->frame_textures[i]);

        ctx->frame_textures[i].Vram =
            gsKit_vram_alloc(ctx->gs_global, tex_size, GSKIT_ALLOC_USERBUFFER);
        if (ctx->frame_textures[i].Vram == GSKIT_ALLOC_ERROR) {
            ps2_video_release_texture(ctx);
            return -1;
        }
        memset(buffer, 0, tex_size);
    }

    ctx->tex_width = tex_w;
    ctx->tex_height = tex_h;
    ctx->frame_width = frame_width;
    ctx->frame_height = frame_height;
    ctx->ready = 1;
    ctx->current_upload = 0;
    ctx->current_display = -1;

    return 0;
}

void ps2_video_upload_frame(Ps2VideoContext *ctx, const u32 *rgba) {
    if (!ctx || !ctx->ready || !rgba)
        return;

    int upload_index = ctx->current_upload & 1;
    GSTEXTURE *texture = &ctx->frame_textures[upload_index];
    u32 *dst = (u32 *)texture->Mem;
    u32 pitch = ctx->tex_width;

    for (u32 y = 0; y < ctx->frame_height; ++y) {
        u32 *row_dst = dst + y * pitch;
        const u32 *row_src = rgba + y * ctx->frame_width;
        memcpy(row_dst, row_src, ctx->frame_width * sizeof(u32));
        if (ctx->frame_width < ctx->tex_width) {
            memset(row_dst + ctx->frame_width, 0,
                   (ctx->tex_width - ctx->frame_width) * sizeof(u32));
        }
    }

    for (u32 y = ctx->frame_height; y < ctx->tex_height; ++y) {
        u32 *row_dst = dst + y * pitch;
        memset(row_dst, 0, ctx->tex_width * sizeof(u32));
    }

    gsKit_texture_upload(ctx->gs_global, texture);

    ctx->current_display = upload_index;
    ctx->current_upload = (upload_index ^ 1);
}

void ps2_video_present(Ps2VideoContext *ctx) {
    if (!ctx || !ctx->ready || ctx->current_display < 0)
        return;

    gsKit_clear(ctx->gs_global, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));

    GSTEXTURE *texture = &ctx->frame_textures[ctx->current_display];

    gsKit_prim_sprite_texture(ctx->gs_global, texture,
                              0.0f, 0.0f,
                              0.0f, 0.0f,
                              (float)ctx->frame_width, (float)ctx->frame_height,
                              (float)ctx->frame_width, (float)ctx->frame_height,
                              0.0f, GS_SETREG_RGBAQ(128, 128, 128, 128, 0));

    gsKit_queue_exec(ctx->gs_global);
    gsKit_sync_flip(ctx->gs_global);
}

void ps2_video_shutdown(Ps2VideoContext *ctx) {
    if (!ctx)
        return;

    ps2_video_release_texture(ctx);
}
