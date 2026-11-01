/**
 * @file screen_boot.c
 * P0 启动页：Logo + 自检进度，2.2s 后进入主页，可点击跳过。
 */

#include "screen_boot.h"

#include "icon.h"
#include "ui.h"
#include "ui_progress.h"
#include "ui_theme.h"

typedef struct {
    lv_obj_t * bar;
    lv_obj_t * status;
    lv_anim_t anim;
    bool done;
} boot_ctx_t;

static void boot_anim_cb(void * var, int32_t value)
{
    boot_ctx_t * ctx = var;
    ui_progress_set_value(ctx->bar, value);
    lv_label_set_text_fmt(ctx->status, "自检 %d%%", (int)value);
}

static void boot_finish(boot_ctx_t * ctx)
{
    if(ctx->done) {
        return;
    }
    ctx->done = true;
    ui_goto(UI_SCREEN_HOME, false);
    ui_after_boot();
}

static void boot_anim_done_cb(lv_anim_t * anim)
{
    boot_finish((boot_ctx_t *)anim->var);
}

static void boot_click_cb(lv_event_t * e)
{
    lv_obj_t * root = lv_event_get_target(e);
    boot_ctx_t * ctx = lv_obj_get_user_data(root);

    if(ctx == NULL) {
        return;
    }
    lv_anim_delete(ctx, NULL);
    boot_finish(ctx);
}

static void boot_delete_cb(lv_event_t * e)
{
    lv_obj_t * root = lv_event_get_target(e);
    boot_ctx_t * ctx = lv_obj_get_user_data(root);

    if(ctx == NULL) {
        return;
    }
    lv_anim_delete(ctx, NULL);
    lv_free(ctx);
    lv_obj_set_user_data(root, NULL);
}

void screen_boot_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    boot_ctx_t * ctx = lv_malloc(sizeof(boot_ctx_t));
    lv_memzero(ctx, sizeof(boot_ctx_t));
    lv_obj_set_user_data(root, ctx);

    lv_obj_t * logo = lv_image_create(root);
    lv_image_set_src(logo, &logo_icon);
    lv_obj_remove_flag(logo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -44);

    lv_obj_t * title = ui_text_create(root, "AIRFORGE", UI_FONT_TITLE, theme->text, 0, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t * sub = ui_text_create(root, "焊接废气净化装置", UI_FONT_TITLE,
                                    theme->muted, 0, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 34);

    ctx->bar = ui_progress_create(root, 40, 186, 160, 10, 10, 0);

    ctx->status = ui_text_create(root, "自检 0%", UI_FONT_TITLE, theme->muted, 0, 0);
    lv_obj_align(ctx->status, LV_ALIGN_TOP_MID, 0, 202);

    lv_obj_t * hint = ui_text_create(root, "点击跳过", UI_FONT_TITLE, theme->muted, 0, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -24);

    lv_anim_init(&ctx->anim);
    lv_anim_set_var(&ctx->anim, ctx);
    lv_anim_set_values(&ctx->anim, 0, 100);
    lv_anim_set_duration(&ctx->anim, 2200);
    lv_anim_set_exec_cb(&ctx->anim, boot_anim_cb);
    lv_anim_set_path_cb(&ctx->anim, lv_anim_path_linear);
    lv_anim_set_completed_cb(&ctx->anim, boot_anim_done_cb);
    lv_anim_start(&ctx->anim);

    lv_obj_add_event_cb(root, boot_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(root, boot_delete_cb, LV_EVENT_DELETE, NULL);
}
