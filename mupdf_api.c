#include "mupdf_api.h"
#include <mupdf/fitz.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 内部结构定义 */
struct MuPdfContext {
    fz_context* ctx;
    char last_error[256];
};

struct MuPdfDocument {
    fz_document* doc;
};

/* === 实现 === */

MUPDF_API MuPdfContext* mupdf_ctx_create(void) {
    MuPdfContext* mctx = (MuPdfContext*)calloc(1, sizeof(MuPdfContext));
    if (!mctx) return NULL;

    mctx->ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
    if (!mctx->ctx) {
        free(mctx);
        return NULL;
    }

    fz_register_document_handlers(mctx->ctx);
    return mctx;
}

MUPDF_API MuPdfDocument* mupdf_doc_open(MuPdfContext* ctx, const char* filepath) {
    if (!ctx || !filepath) return NULL;

    MuPdfDocument* doc = (MuPdfDocument*)calloc(1, sizeof(MuPdfDocument));
    if (!doc) return NULL;

    fz_try(ctx->ctx) {
        doc->doc = fz_open_document(ctx->ctx, filepath);
    } fz_catch(ctx->ctx) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", fz_caught_message(ctx->ctx));
        free(doc);
        return NULL;
    }

    return doc;
}

MUPDF_API int mupdf_doc_page_count(MuPdfContext* ctx, MuPdfDocument* doc) {
    if (!ctx || !doc) return 0;

    int count = 0;
    fz_try(ctx->ctx) {
        count = fz_count_pages(ctx->ctx, doc->doc);
    } fz_catch(ctx->ctx) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", fz_caught_message(ctx->ctx));
        return -1;
    }
    return count;
}

/* === JSON 序列化 === */

/* 动态字符串 builder */
typedef struct {
    char* buf;
    size_t len;
    size_t cap;
} JsonBuf;

static int json_init(JsonBuf* jb, size_t cap) {
    jb->buf = (char*)malloc(cap);
    if (!jb->buf) return 0;
    jb->buf[0] = '\0';
    jb->len = 0;
    jb->cap = cap;
    return 1;
}

static int json_ensure(JsonBuf* jb, size_t extra) {
    if (jb->len + extra + 1 <= jb->cap) return 1;
    size_t new_cap = jb->cap * 2;
    while (jb->len + extra + 1 > new_cap) new_cap *= 2;
    char* new_buf = (char*)realloc(jb->buf, new_cap);
    if (!new_buf) return 0;
    jb->buf = new_buf;
    jb->cap = new_cap;
    return 1;
}

static int json_append_raw(JsonBuf* jb, const char* s) {
    size_t n = strlen(s);
    if (!json_ensure(jb, n)) return 0;
    memcpy(jb->buf + jb->len, s, n + 1);
    jb->len += n;
    return 1;
}

static int json_append_char(JsonBuf* jb, char c) {
    if (!json_ensure(jb, 1)) return 0;
    jb->buf[jb->len++] = c;
    jb->buf[jb->len] = '\0';
    return 1;
}

/* 将字符串转义为 JSON 字符串（含引号） */
static int json_append_string(JsonBuf* jb, const char* s) {
    if (!s) {
        return json_append_raw(jb, "\"\"");
    }
    if (!json_append_char(jb, '"')) return 0;
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        switch (*p) {
            case '"':  if (!json_append_raw(jb, "\\\"")) return 0; break;
            case '\\': if (!json_append_raw(jb, "\\\\")) return 0; break;
            case '\b': if (!json_append_raw(jb, "\\b")) return 0; break;
            case '\f': if (!json_append_raw(jb, "\\f")) return 0; break;
            case '\n': if (!json_append_raw(jb, "\\n")) return 0; break;
            case '\r': if (!json_append_raw(jb, "\\r")) return 0; break;
            case '\t': if (!json_append_raw(jb, "\\t")) return 0; break;
            default:
                if (*p < 0x20) {
                    char hex[7];
                    snprintf(hex, sizeof(hex), "\\u%04x", *p);
                    if (!json_append_raw(jb, hex)) return 0;
                } else {
                    if (!json_append_char(jb, (char)*p)) return 0;
                }
                break;
        }
        p++;
    }
    if (!json_append_char(jb, '"')) return 0;
    return 1;
}

static int json_append_int(JsonBuf* jb, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return json_append_raw(jb, buf);
}

/* 递归序列化 fz_outline 节点为 JSON */
static int outline_to_json(JsonBuf* jb, fz_outline* node) {
    if (!node) return 1;

    int first = 1;
    for (fz_outline* cur = node; cur; cur = cur->next) {
        if (!first) {
            if (!json_append_raw(jb, ",")) return 0;
        }
        first = 0;

        if (!json_append_raw(jb, "{")) return 0;

        if (!json_append_raw(jb, "\"title\":")) return 0;
        if (!json_append_string(jb, cur->title)) return 0;

        if (!json_append_raw(jb, ",\"uri\":")) return 0;
        if (!json_append_string(jb, cur->uri)) return 0;

        if (!json_append_raw(jb, ",\"page\":")) return 0;
        if (!json_append_int(jb, cur->page.page)) return 0;

        if (!json_append_raw(jb, ",\"isOpen\":")) return 0;
        if (!json_append_raw(jb, cur->is_open ? "true" : "false")) return 0;

        if (!json_append_raw(jb, ",\"flags\":")) return 0;
        if (!json_append_int(jb, cur->flags)) return 0;

        if (!json_append_raw(jb, ",\"children\":[")) return 0;
        if (cur->down) {
            if (!outline_to_json(jb, cur->down)) return 0;
        }
        if (!json_append_raw(jb, "]}")) return 0;
    }
    return 1;
}

MUPDF_API MuPdfOutlineJson* mupdf_doc_get_outline(MuPdfContext* ctx, MuPdfDocument* doc) {
    if (!ctx || !doc) return NULL;

    fz_outline* outline = NULL;
    MuPdfOutlineJson* result = NULL;
    JsonBuf jb = {0};

    fz_try(ctx->ctx) {
        outline = fz_load_outline(ctx->ctx, doc->doc);

        if (!json_init(&jb, 4096)) {
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            break;
        }

        if (!json_append_raw(&jb, "[")) {
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            break;
        }

        if (outline) {
            if (!outline_to_json(&jb, outline)) {
                snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
                break;
            }
        }

        if (!json_append_raw(&jb, "]")) {
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            break;
        }

        result = (MuPdfOutlineJson*)calloc(1, sizeof(MuPdfOutlineJson));
        if (!result) {
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            break;
        }
        result->json = jb.buf;
        result->length = (int)jb.len;
        jb.buf = NULL; /* result 接管内存 */

    } fz_catch(ctx->ctx) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", fz_caught_message(ctx->ctx));
    }

    if (outline) fz_drop_outline(ctx->ctx, outline);
    if (jb.buf) free(jb.buf);
    return result;
}

MUPDF_API void mupdf_outline_free(MuPdfOutlineJson* outline) {
    if (!outline) return;
    if (outline->json) free(outline->json);
    free(outline);
}

MUPDF_API MuPdfImage* mupdf_page_render(MuPdfContext* ctx, MuPdfDocument* doc,
                                         int page_number, float zoom, float rotate, int alpha) {
    if (!ctx || !doc) return NULL;

    fz_matrix ctm = fz_scale(zoom / 100.0f, zoom / 100.0f);
    ctm = fz_pre_rotate(ctm, rotate);
    fz_colorspace* cs = fz_device_rgb(ctx->ctx);

    fz_pixmap* pix = NULL;
    fz_try(ctx->ctx) {
        pix = fz_new_pixmap_from_page_number(ctx->ctx, doc->doc, page_number, ctm, cs, alpha);
    } fz_catch(ctx->ctx) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", fz_caught_message(ctx->ctx));
        return NULL;
    }

    MuPdfImage* image = (MuPdfImage*)calloc(1, sizeof(MuPdfImage));
    if (!image) {
        fz_drop_pixmap(ctx->ctx, pix);
        return NULL;
    }

    image->width = fz_pixmap_width(ctx->ctx, pix);
    image->height = fz_pixmap_height(ctx->ctx, pix);
    image->stride = fz_pixmap_stride(ctx->ctx, pix);
    image->components = fz_pixmap_components(ctx->ctx, pix);

    /* 复制像素数据到独立 buffer（因为 pixmap 销毁后 samples 无效） */
    size_t data_size = (size_t)image->stride * image->height;
    image->buffer = (unsigned char*)malloc(data_size);
    if (!image->buffer) {
        fz_drop_pixmap(ctx->ctx, pix);
        free(image);
        return NULL;
    }
    memcpy(image->buffer, fz_pixmap_samples(ctx->ctx, pix), data_size);

    fz_drop_pixmap(ctx->ctx, pix);
    return image;
}

MUPDF_API void mupdf_doc_close(MuPdfContext* ctx, MuPdfDocument* doc) {
    if (!ctx || !doc) return;
    if (doc->doc) fz_drop_document(ctx->ctx, doc->doc);
    free(doc);
}

MUPDF_API void mupdf_ctx_destroy(MuPdfContext* ctx) {
    if (!ctx) return;
    if (ctx->ctx) fz_drop_context(ctx->ctx);
    free(ctx);
}

MUPDF_API void mupdf_image_free(MuPdfImage* image) {
    if (!image) return;
    if (image->buffer) free(image->buffer);
    free(image);
}

/* === UTF-8 辅助函数 === */

/* 将 Unicode 码点编码为 UTF-8，返回写入字节数（不含结束符） */
static int unicode_to_utf8(int c, char out[5]) {
    if (c < 0x80) {
        out[0] = (char)c;
        out[1] = '\0';
        return 1;
    } else if (c < 0x800) {
        out[0] = (char)(0xC0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 0x3F));
        out[2] = '\0';
        return 2;
    } else if (c < 0x10000) {
        out[0] = (char)(0xE0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[2] = (char)(0x80 | (c & 0x3F));
        out[3] = '\0';
        return 3;
    } else {
        out[0] = (char)(0xF0 | (c >> 18));
        out[1] = (char)(0x80 | ((c >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((c >> 6) & 0x3F));
        out[3] = (char)(0x80 | (c & 0x3F));
        out[4] = '\0';
        return 4;
    }
}

/* === 结构化文本提取 === */

MUPDF_API MuPdfTextPage* mupdf_page_get_stext(MuPdfContext* ctx, MuPdfDocument* doc,
                                               int page_number) {
    if (!ctx || !doc) return NULL;

    fz_page* page = NULL;
    fz_stext_page* stext_page = NULL;
    fz_device* dev = NULL;
    MuPdfTextPage* result = NULL;

    /* MuPDF API 调用放在 fz_try 中（可能 longjmp） */
    fz_try(ctx->ctx) {
        page = fz_load_page(ctx->ctx, doc->doc, page_number);
        stext_page = fz_new_stext_page(ctx->ctx, fz_empty_rect);

        fz_stext_options opts = { 0 };
        opts.flags = FZ_STEXT_PRESERVE_WHITESPACE;

        dev = fz_new_stext_device(ctx->ctx, stext_page, &opts);
        fz_run_page(ctx->ctx, page, dev, fz_identity, NULL);
        fz_close_device(ctx->ctx, dev);
        fz_drop_device(ctx->ctx, dev);
        dev = NULL;
    } fz_catch(ctx->ctx) {
        snprintf(ctx->last_error, sizeof(ctx->last_error), "%s", fz_caught_message(ctx->ctx));
        if (dev) fz_drop_device(ctx->ctx, dev);
        if (stext_page) fz_drop_stext_page(ctx->ctx, stext_page);
        if (page) fz_drop_page(ctx->ctx, page);
        return NULL;
    }

    /* 数据转换在 fz_try 外部进行，避免 longjmp 导致 malloc 泄漏 */
    {
        int block_count = 0;
        int* line_counts = NULL;

        /* 第一遍：统计文本块数 */
        for (fz_stext_block* blk = stext_page->first_block; blk; blk = blk->next) {
            if (blk->type == FZ_STEXT_BLOCK_TEXT)
                block_count++;
        }

        if (block_count == 0) {
            result = (MuPdfTextPage*)calloc(1, sizeof(MuPdfTextPage));
            goto done;
        }

        /* 统计每个块的行数 */
        line_counts = (int*)calloc((size_t)block_count, sizeof(int));
        if (!line_counts) {
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            goto done;
        }

        {
            int bi = 0;
            for (fz_stext_block* blk = stext_page->first_block; blk; blk = blk->next) {
                if (blk->type != FZ_STEXT_BLOCK_TEXT) continue;
                for (fz_stext_line* ln = blk->u.t.first_line; ln; ln = ln->next)
                    line_counts[bi]++;
                bi++;
            }
        }

        /* 分配顶层结构 */
        result = (MuPdfTextPage*)calloc(1, sizeof(MuPdfTextPage));
        if (!result) {
            free(line_counts);
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            goto done;
        }

        result->blocks_count = block_count;
        result->blocks = (MuPdfTextBlock*)calloc((size_t)block_count, sizeof(MuPdfTextBlock));
        if (!result->blocks) {
            free(line_counts);
            mupdf_stext_page_free(result);
            result = NULL;
            snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
            goto done;
        }

        /* 第二遍：填充数据 */
        {
            int bi = 0;
            for (fz_stext_block* blk = stext_page->first_block; blk; blk = blk->next) {
                if (blk->type != FZ_STEXT_BLOCK_TEXT) continue;

                MuPdfTextBlock* tb = &result->blocks[bi];
                tb->bbox.x0 = blk->bbox.x0;
                tb->bbox.y0 = blk->bbox.y0;
                tb->bbox.x1 = blk->bbox.x1;
                tb->bbox.y1 = blk->bbox.y1;
                tb->lines_count = line_counts[bi];
                tb->lines = (MuPdfTextLine*)calloc((size_t)line_counts[bi], sizeof(MuPdfTextLine));
                if (!tb->lines) {
                    free(line_counts);
                    mupdf_stext_page_free(result);
                    result = NULL;
                    snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
                    goto done;
                }

                int li = 0;
                for (fz_stext_line* ln = blk->u.t.first_line; ln; ln = ln->next) {
                    MuPdfTextLine* tl = &tb->lines[li];

                    tl->bbox.x0 = ln->bbox.x0;
                    tl->bbox.y0 = ln->bbox.y0;
                    tl->bbox.x1 = ln->bbox.x1;
                    tl->bbox.y1 = ln->bbox.y1;

                    /* 统计字符数 */
                    int ch_count = 0;
                    for (fz_stext_char* ch = ln->first_char; ch; ch = ch->next)
                        ch_count++;

                    tl->chars_count = ch_count;
                    tl->chars = (MuPdfTextChar*)calloc((size_t)ch_count, sizeof(MuPdfTextChar));
                    tl->text = (char*)calloc((size_t)(ch_count * 4 + 1), 1);

                    if (!tl->chars || !tl->text) {
                        free(line_counts);
                        mupdf_stext_page_free(result);
                        result = NULL;
                        snprintf(ctx->last_error, sizeof(ctx->last_error), "out of memory");
                        goto done;
                    }

                    /* 填充字符数据 */
                    int ci = 0;
                    size_t text_pos = 0;
                    for (fz_stext_char* ch = ln->first_char; ch; ch = ch->next) {
                        MuPdfTextChar* tc = &tl->chars[ci];

                        fz_rect r = fz_rect_from_quad(ch->quad);
                        tc->bbox.x0 = r.x0;
                        tc->bbox.y0 = r.y0;
                        tc->bbox.x1 = r.x1;
                        tc->bbox.y1 = r.y1;

                        int bytes = unicode_to_utf8(ch->c, tc->utf8);
                        if (bytes > 0) {
                            memcpy(tl->text + text_pos, tc->utf8, (size_t)bytes);
                            text_pos += (size_t)bytes;
                        }
                        ci++;
                    }
                    tl->text[text_pos] = '\0';
                    li++;
                }
                bi++;
            }
        }
        free(line_counts);
    }

done:
    if (page) fz_drop_page(ctx->ctx, page);
    if (stext_page) fz_drop_stext_page(ctx->ctx, stext_page);

    return result;
}

MUPDF_API void mupdf_stext_page_free(MuPdfTextPage* page) {
    if (!page) return;
    if (page->blocks) {
        for (int bi = 0; bi < page->blocks_count; bi++) {
            MuPdfTextBlock* tb = &page->blocks[bi];
            if (tb->lines) {
                for (int li = 0; li < tb->lines_count; li++) {
                    MuPdfTextLine* tl = &tb->lines[li];
                    if (tl->chars) free(tl->chars);
                    if (tl->text) free(tl->text);
                }
                free(tb->lines);
            }
        }
        free(page->blocks);
    }
    free(page);
}

MUPDF_API const char* mupdf_last_error(MuPdfContext* ctx) {
    if (!ctx) return "NULL context";
    return ctx->last_error;
}
