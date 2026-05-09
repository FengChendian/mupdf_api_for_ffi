#ifndef MUPDF_API_H
#define MUPDF_API_H

#ifdef _WIN32
    #ifdef MUPDF_API_EXPORTS
        #define MUPDF_API __declspec(dllexport)
    #else
        #define MUPDF_API __declspec(dllimport)
    #endif
#else
    #define MUPDF_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明句柄类型 */
typedef struct MuPdfContext MuPdfContext;
typedef struct MuPdfDocument MuPdfDocument;

/* 错误码 */
typedef enum {
    MUPDF_OK = 0,
    MUPDF_ERROR_GENERIC,
    MUPDF_ERROR_ARGUMENT,
    MUPDF_ERROR_OOM,
} MuPdfError;

/* 渲染结果 - 类似 PDFium 的数据布局 */
typedef struct {
    int width;
    int height;
    int stride;      /* 每行字节数 */
    int components;  /* 每像素分量数 (RGB=3, RGBA=4) */
    unsigned char* buffer;  /* 原始字节数据，调用方负责释放 */
} MuPdfImage;

/* 目录项（树形结构） */
typedef struct MuPdfOutlineItem {
    char* title;           /* 标题文本 */
    char* uri;             /* 链接 URI */
    int page;              /* 页码 (-1 表示外部链接或无目标) */
    int level;             /* 层级深度 (从 0 开始) */
    int is_open;           /* 是否展开 */
    int flags;             /* 1=粗体，2=斜体 */
    struct MuPdfOutlineItem* next;  /* 同级下一项 */
    struct MuPdfOutlineItem* down;  /* 子项 */
} MuPdfOutlineItem;

/* 目录 JSON 结果 */
typedef struct {
    char* json;    /* UTF-8 JSON 字符串，调用方负责释放 */
    int length;    /* 字符串字节长度 (不含末尾 \0) */
} MuPdfOutlineJson;

/* === 生命周期管理 === */

/* 创建上下文（内部处理 fz_register_document_handlers） */
MUPDF_API MuPdfContext* mupdf_ctx_create(void);

/* 打开文档
   返回 NULL 表示失败，可通过 mupdf_last_error 获取错误信息 */
MUPDF_API MuPdfDocument* mupdf_doc_open(MuPdfContext* ctx, const char* filepath);

/* 获取页面数量 */
MUPDF_API int mupdf_doc_page_count(MuPdfContext* ctx, MuPdfDocument* doc);

/* 获取目录（TOC）
   返回 JSON 字符串结构，调用方负责通过 mupdf_outline_free 释放 */
MUPDF_API MuPdfOutlineJson* mupdf_doc_get_outline(MuPdfContext* ctx, MuPdfDocument* doc);

/* 释放目录 JSON 结构 */
MUPDF_API void mupdf_outline_free(MuPdfOutlineJson* outline);

/* 渲染页面
   page_number: 从 0 开始
   zoom: 缩放百分比 (100 = 72dpi)
   rotate: 旋转角度 (0, 90, 180, 270)
   alpha: 是否包含 alpha 通道 (0 或 1)
   返回 NULL 表示失败 */
MUPDF_API MuPdfImage* mupdf_page_render(MuPdfContext* ctx, MuPdfDocument* doc,
                                         int page_number, float zoom, float rotate, int alpha);

/* 关闭文档（释放文档和内部 context 资源） */
MUPDF_API void mupdf_doc_close(MuPdfContext* ctx, MuPdfDocument* doc);

/* 销毁上下文 */
MUPDF_API void mupdf_ctx_destroy(MuPdfContext* ctx);

/* 释放渲染结果 */
MUPDF_API void mupdf_image_free(MuPdfImage* image);

/* 获取最后错误信息（返回静态字符串，不要释放） */
MUPDF_API const char* mupdf_last_error(MuPdfContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* MUPDF_API_H */
