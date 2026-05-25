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

/* 矩形 */
typedef struct {
    float x0, y0, x1, y1;
} MuPdfRect;

/* 渲染结果 - 类似 PDFium 的数据布局 */
typedef struct {
    int width;
    int height;
    int stride;      /* 每行字节数 */
    int components;  /* 每像素分量数 (始终为 RGBA=4) */
    unsigned char* buffer;  /* 原始字节数据，调用方负责释放 */
} MuPdfImage;

/* 多页渲染结果 */
typedef struct {
    int images_count;
    MuPdfImage** images;  /* 图片数组，调用方负责释放 */
} MuPdfImages;

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

/* 结构化文本提取 */
typedef struct {
    MuPdfRect bbox;
    char utf8[5];        /* UTF-8 编码的单个字符 + 结束符 */
} MuPdfTextChar;

typedef struct {
    MuPdfRect bbox;
    int chars_count;
    MuPdfTextChar* chars;  /* 字符数组 */
    char* text;            /* 整行文字，调用方负责释放 */
} MuPdfTextLine;

typedef struct {
    MuPdfRect bbox;
    int lines_count;
    MuPdfTextLine* lines;  /* 行数组 */
} MuPdfTextBlock;

typedef struct {
    int blocks_count;
    MuPdfTextBlock* blocks;  /* 块数组 */
} MuPdfTextPage;

/* 页面注释 */
typedef struct {
    MuPdfRect rect;
    int type;             /* pdf_annot_type 枚举值 (PDF_ANNOT_HIGHLIGHT, PDF_ANNOT_TEXT 等) */
} MuPdfAnnotation;

typedef struct {
    int annots_count;
    MuPdfAnnotation* annots;  /* 注释数组 */
} MuPdfAnnotationPage;

/* 页面链接 */
typedef struct {
    MuPdfRect rect;
    char* uri;                 /* 链接 URI，调用方负责释放 */
} MuPdfLink;

typedef struct {
    int links_count;
    MuPdfLink* links;          /* 链接数组 */
} MuPdfLinkPage;

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
   alpha: 0=不透明(RGB输入,自动转为RGBA), 1=含透明通道(RGBA)
   返回的 MuPdfImage 始终为 RGBA 格式 (components=4, stride=width*4)
   返回 NULL 表示失败 */
MUPDF_API MuPdfImage* mupdf_page_render(MuPdfContext* ctx, MuPdfDocument* doc,
                                         int page_number, float zoom, float rotate, int alpha);

/* 渲染页面内容（不含注释/annotations）
   参数和返回格式同 mupdf_page_render，但不渲染批注、高亮等注释内容 */
MUPDF_API MuPdfImage* mupdf_page_render_no_annot(MuPdfContext* ctx, MuPdfDocument* doc,
                                                  int page_number, float zoom, float rotate, int alpha);

/* 关闭文档（释放文档和内部 context 资源） */
MUPDF_API void mupdf_doc_close(MuPdfContext* ctx, MuPdfDocument* doc);

/* 销毁上下文 */
MUPDF_API void mupdf_ctx_destroy(MuPdfContext* ctx);

/* 渲染页面范围（不含注释），返回所有页面的图片数组
   start_page: 起始页（从 0 开始）
   end_page: 结束页（含，从 0 开始）
   其他参数同 mupdf_page_render_no_annot */
MUPDF_API MuPdfImages* mupdf_pages_render_no_annot(MuPdfContext* ctx, MuPdfDocument* doc,
                                                     int start_page, int end_page,
                                                     float zoom, float rotate, int alpha);

/* 释放渲染结果 */
MUPDF_API void mupdf_image_free(MuPdfImage* image);

/* 释放多页渲染结果 */
MUPDF_API void mupdf_images_free(MuPdfImages* images);

/* 提取页面结构化文本
   page_number: 从 0 开始
   返回 NULL 表示失败 */
MUPDF_API MuPdfTextPage* mupdf_page_get_stext(MuPdfContext* ctx, MuPdfDocument* doc,
                                               int page_number);

/* 释放结构化文本提取结果 */
MUPDF_API void mupdf_stext_page_free(MuPdfTextPage* page);

/* 获取页面注释列表
   page_number: 从 0 开始
   返回 NULL 表示失败 */
MUPDF_API MuPdfAnnotationPage* mupdf_page_get_annots(MuPdfContext* ctx, MuPdfDocument* doc,
                                                       int page_number);

/* 释放注释列表 */
MUPDF_API void mupdf_annot_page_free(MuPdfAnnotationPage* page);

/* 添加注释到页面
   type: pdf_annot_type 枚举值 (如 PDF_ANNOT_HIGHLIGHT)
   rect: 注释矩形坐标
   返回 0 表示成功，-1 表示失败 */
MUPDF_API int mupdf_page_add_annot(MuPdfContext* ctx, MuPdfDocument* doc,
                                    int page_number, int type, MuPdfRect rect);

/* 按索引删除注释
   index: 从 0 开始
   返回 0 表示成功，-1 表示失败 */
MUPDF_API int mupdf_page_delete_annot(MuPdfContext* ctx, MuPdfDocument* doc,
                                       int page_number, int index);

/* 获取页面链接列表
   page_number: 从 0 开始
   返回 NULL 表示失败 */
MUPDF_API MuPdfLinkPage* mupdf_page_get_links(MuPdfContext* ctx, MuPdfDocument* doc,
                                                int page_number);

/* 释放链接列表 */
MUPDF_API void mupdf_link_page_free(MuPdfLinkPage* page);

/* 保存文档到文件（持久化注释/链接修改）
   返回 0 表示成功，-1 表示失败 */
MUPDF_API int mupdf_doc_save(MuPdfContext* ctx, MuPdfDocument* doc, const char* filepath);

/* 获取最后错误信息（返回静态字符串，不要释放） */
MUPDF_API const char* mupdf_last_error(MuPdfContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* MUPDF_API_H */
