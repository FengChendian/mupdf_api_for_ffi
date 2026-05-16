#include <stdio.h>
#include <stdlib.h>
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

            /* 打印用户要求的关键信息：第一行第一个字符，第二行第二个字符 */
            // printf("\n    === 关键信息 ===\n");
            // if (stext->blocks_count > 0 && stext->blocks[0].lines_count > 0) {
            //     MuPdfTextLine* line0 = &stext->blocks[0].lines[0];
            //     if (line0->chars_count > 0) {
            //         MuPdfTextChar* ch0 = &line0->chars[0];
            //         printf("    第一行第一个字符: '%s'  bbox=[%.2f, %.2f, %.2f, %.2f]\n",
            //                ch0->utf8, ch0->bbox.x0, ch0->bbox.y0, ch0->bbox.x1, ch0->bbox.y1);
            //     } else {
            //         printf("    第一行没有字符\n");
            //     }
            // } else {
            //     printf("    没有第一行\n");
            // }

            // if (stext->blocks_count > 0 && stext->blocks[0].lines_count > 1) {
            //     MuPdfTextLine* line1 = &stext->blocks[0].lines[1];
            //     if (line1->chars_count > 1) {
            //         MuPdfTextChar* ch1 = &line1->chars[1];
            //         printf("    第二行第二个字符: '%s'  bbox=[%.2f, %.2f, %.2f, %.2f]\n",
            //                ch1->utf8, ch1->bbox.x0, ch1->bbox.y0, ch1->bbox.x1, ch1->bbox.y1);
            //     } else {
            //         printf("    第二行字符数不足 %d 个\n", line1->chars_count > 0 ? 2 : 1);
            //     }
            // } else {
            //     printf("    没有第二行\n");
            // }

            mupdf_stext_page_free(stext);
            printf("\n    Structured text freed.\n");
        }
    } else {
        printf("    Skipping stext test (no pages)\n");
    }

    /* 清理资源 */
    printf("\n[7] Cleaning up...\n");
    mupdf_doc_close(ctx, doc);
    printf("    Document closed.\n");
    mupdf_ctx_destroy(ctx);
    printf("    Context destroyed.\n");

    printf("\n=== All tests passed! ===\n");
    return 0;
}
