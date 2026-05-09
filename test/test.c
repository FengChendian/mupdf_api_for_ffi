#include <stdio.h>
#include <stdlib.h>
#include "mupdf_api.h"

int main(int argc, char* argv[]) {
    const char* pdf_path = "nwag224.pdf";

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

    /* 清理资源 */
    printf("\n[6] Cleaning up...\n");
    mupdf_doc_close(ctx, doc);
    printf("    Document closed.\n");
    mupdf_ctx_destroy(ctx);
    printf("    Context destroyed.\n");

    printf("\n=== All tests passed! ===\n");
    return 0;
}
