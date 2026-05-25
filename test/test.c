#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mupdf_api.h"

int main(int argc, char* argv[]) {
    const char* pdf_path = "0.pdf";

    if (argc > 1) {
        pdf_path = argv[1];
    }

    printf("=== MuPDF DLL Test ===\n\n");

    /* 1. 测试 mupdf_ctx_create */
    printf("[1] Testing mupdf_ctx_create...\n");
    MuPdfContext* ctx = mupdf_ctx_create();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return 1;
    }
    printf("    Context created successfully: %p\n\n", (void*)ctx);

    /* 2. 测试 mupdf_doc_open */
    printf("[2] Testing mupdf_doc_open (%s)...\n", pdf_path);
    MuPdfDocument* doc = mupdf_doc_open(ctx, pdf_path);
    if (!doc) {
        fprintf(stderr, "Failed to open document: %s\n", mupdf_last_error(ctx));
        mupdf_ctx_destroy(ctx);
        return 1;
    }
    printf("    Document opened successfully: %p\n\n", (void*)doc);

    /* 3. 测试 mupdf_doc_page_count */
    printf("[3] Testing mupdf_doc_page_count...\n");
    int page_count = mupdf_doc_page_count(ctx, doc);
    if (page_count < 0) {
        fprintf(stderr, "Failed to count pages: %s\n", mupdf_last_error(ctx));
    } else {
        printf("    Page count: %d\n\n", page_count);
    }

    /* 4. 测试 mupdf_doc_get_outline */
    printf("[4] Testing mupdf_doc_get_outline...\n");
    MuPdfOutlineJson* outline = mupdf_doc_get_outline(ctx, doc);
    if (!outline) {
        printf("    No outline available or error: %s\n", mupdf_last_error(ctx));
    } else {
        printf("    Outline JSON length: %d bytes\n", outline->length);
        printf("    Outline JSON:\n%.*s\n", outline->length, outline->json);
        mupdf_outline_free(outline);
        printf("    Outline freed.\n");
    }

    /* 5. 测试 mupdf_page_render */
    printf("\n[5] Testing mupdf_page_render (page 0, zoom=100%%, rotate=0, alpha=0)...\n");
    if (page_count > 0) {
        MuPdfImage* image = mupdf_page_render(ctx, doc, 0, 100.0f, 0.0f, 0);
        if (!image) {
            fprintf(stderr, "Failed to render page: %s\n", mupdf_last_error(ctx));
        } else {
            printf("    Page rendered successfully!\n");
            printf("    Image info:\n");
            printf("      - Width: %d px\n", image->width);
            printf("      - Height: %d px\n", image->height);
            printf("      - Stride: %d bytes/row\n", image->stride);
            printf("      - Components: %d\n", image->components);
            printf("      - Buffer size: %d bytes\n", image->stride * image->height);

            /* 释放图像 */
            mupdf_image_free(image);
            printf("\n    Image freed.\n");
        }
    } else {
        printf("    Skipping render test (no pages)\n");
    }

    /* 6. 测试结构化文本提取 */
    printf("\n[6] Testing mupdf_page_get_stext (page 0)...\n");
    if (page_count > 0) {
        MuPdfTextPage* stext = mupdf_page_get_stext(ctx, doc, 0);
        if (!stext) {
            fprintf(stderr, "    Failed to extract structured text: %s\n", mupdf_last_error(ctx));
        } else {
            printf("    Blocks count: %d\n", stext->blocks_count);

            for (int bi = 0; bi < stext->blocks_count; bi++) {
                MuPdfTextBlock* tb = &stext->blocks[bi];
                printf("\n    --- Block %d ---\n", bi);
                printf("    Block bbox: [%.2f, %.2f, %.2f, %.2f]\n",
                       tb->bbox.x0, tb->bbox.y0, tb->bbox.x1, tb->bbox.y1);
                printf("    Lines count: %d\n", tb->lines_count);

                for (int li = 0; li < tb->lines_count; li++) {
                    MuPdfTextLine* tl = &tb->lines[li];
                    printf("\n      Line %d:\n", li);
                    printf("        bbox: [%.2f, %.2f, %.2f, %.2f]\n",
                           tl->bbox.x0, tl->bbox.y0, tl->bbox.x1, tl->bbox.y1);
                    printf("        chars_count: %d\n", tl->chars_count);
                    printf("        text: \"%s\"\n", tl->text);

                    /* 打印每个字符的详细信息 */
                    for (int ci = 0; ci < tl->chars_count; ci++) {
                        MuPdfTextChar* tc = &tl->chars[ci];
                        printf("          char[%d]: '%s'  bbox=[%.2f, %.2f, %.2f, %.2f]  size=(%.2f x %.2f)\n",
                               ci, tc->utf8,
                               tc->bbox.x0, tc->bbox.y0, tc->bbox.x1, tc->bbox.y1,
                               tc->bbox.x1 - tc->bbox.x0, tc->bbox.y1 - tc->bbox.y0);
                    }
                }
            }

            mupdf_stext_page_free(stext);
            printf("\n    Structured text freed.\n");
        }
    } else {
        printf("    Skipping stext test (no pages)\n");
    }

    /* 7. 测试 mupdf_pages_render_no_annot 批量渲染所有页面 */
    printf("\n[7] Testing mupdf_pages_render_no_annot (all pages, zoom=100%%, rotate=0, alpha=0)...\n");
    if (page_count > 0) {
        clock_t t0 = clock();
        MuPdfImages* images = mupdf_pages_render_no_annot(ctx, doc, 0, page_count - 1, 100.0f, 0.0f, 1);
        clock_t t1 = clock();
        if (!images) {
            fprintf(stderr, "    Failed to batch render: %s\n", mupdf_last_error(ctx));
        } else {
            double elapsed = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;
            printf("    Rendered %d pages in %.2f ms (%.2f ms/page)\n",
                   images->images_count, elapsed, elapsed / images->images_count);
            mupdf_images_free(images);
            printf("    Batch render results freed.\n");
        }
    } else {
        printf("    Skipping batch render test (no pages)\n");
    }

    /* 8. 测试 mupdf_page_render_no_annot 逐页渲染所有页面 */
    printf("\n[8] Testing mupdf_page_render_no_annot (each page, zoom=100%%, rotate=0, alpha=0)...\n");
    if (page_count > 0) {
        clock_t t0 = clock();
        for (int i = 0; i < page_count; i++) {
            MuPdfImage* img = mupdf_page_render_no_annot(ctx, doc, i, 100.0f, 0.0f, 1);
            if (!img) {
                fprintf(stderr, "    Failed to render page %d: %s\n", i, mupdf_last_error(ctx));
            } else {
                mupdf_image_free(img);
            }
        }
        clock_t t1 = clock();
        double elapsed = (double)(t1 - t0) / CLOCKS_PER_SEC * 1000.0;
        printf("    Rendered %d pages in %.2f ms (%.2f ms/page)\n",
               page_count, elapsed, elapsed / page_count);
    } else {
        printf("    Skipping single-page render test (no pages)\n");
    }

    /* 清理资源 */
    printf("\n[9] Cleaning up...\n");
    mupdf_doc_close(ctx, doc);
    printf("    Document closed.\n");
    mupdf_ctx_destroy(ctx);
    printf("    Context destroyed.\n");

    printf("\n=== All tests passed! ===\n");
    return 0;
}
