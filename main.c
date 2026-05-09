

#include "mupdf_api.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	char *input;
	float zoom, rotate;
	int page_count;
	MuPdfContext* ctx;
	MuPdfDocument* doc;
	MuPdfImage* img;

	if (argc < 3)
	{
		fprintf(stderr, "usage: example input-file page-number [ zoom [ rotate ] ]\n");
		fprintf(stderr, "\tinput-file: path of PDF, XPS, CBZ or EPUB document to open\n");
		fprintf(stderr, "\tPage numbering starts from one.\n");
		fprintf(stderr, "\tZoom level is in percent (100 percent is 72 dpi).\n");
		fprintf(stderr, "\tRotation is in degrees clockwise.\n");
		return EXIT_FAILURE;
	}

	input = argv[1];
	int page_number = atoi(argv[2]) - 1;
	zoom = argc > 3 ? atof(argv[3]) : 100;
	rotate = argc > 4 ? atof(argv[4]) : 0;

	/* Create context (internal handles fz_register_document_handlers) */
	ctx = mupdf_ctx_create();
	if (!ctx)
	{
		fprintf(stderr, "cannot create mupdf context\n");
		return EXIT_FAILURE;
	}

	/* Open the document. */
	doc = mupdf_doc_open(ctx, input);
	if (!doc)
	{
		fprintf(stderr, "cannot open document: %s\n", mupdf_last_error(ctx));
		mupdf_ctx_destroy(ctx);
		return EXIT_FAILURE;
	}

	/* Count the number of pages. */
	page_count = mupdf_doc_page_count(ctx, doc);
	if (page_count < 0)
	{
		fprintf(stderr, "cannot count pages: %s\n", mupdf_last_error(ctx));
		mupdf_doc_close(ctx, doc);
		mupdf_ctx_destroy(ctx);
		return EXIT_FAILURE;
	}

	/* Test the exported mupdf_doc_get_outline function */
	MuPdfOutlineJson *outline_json = mupdf_doc_get_outline(ctx, doc);
	if (outline_json) {
		printf("Outline JSON:\n%s\n\n", outline_json->json);
		mupdf_outline_free(outline_json);
	} else {
		printf("No outline or failed: %s\n\n", mupdf_last_error(ctx));
	} 

	if (page_number < 0 || page_number >= page_count)
	{
		fprintf(stderr, "page number out of range: %d (page count %d)\n", page_number + 1, page_count);
		mupdf_doc_close(ctx, doc);
		mupdf_ctx_destroy(ctx);
		return EXIT_FAILURE;
	}

	/* Render page using the exported mupdf_page_render function */
	img = mupdf_page_render(ctx, doc, page_number, zoom, rotate, 0);
	if (!img)
	{
		fprintf(stderr, "cannot render page: %s\n", mupdf_last_error(ctx));
		mupdf_doc_close(ctx, doc);
		mupdf_ctx_destroy(ctx);
		return EXIT_FAILURE;
	}

	/* Print image data */
	printf("%d %d\n", img->width, img->height);
	printf("255\n");
	for (int y = 0; y < img->height; ++y)
	{
		unsigned char *p = &img->buffer[y * img->stride];
		for (int x = 0; x < img->width; ++x)
		{
			p += img->components;
		}
	}

	/* Clean up. */
	mupdf_image_free(img);
	mupdf_doc_close(ctx, doc);
	mupdf_ctx_destroy(ctx);
	return EXIT_SUCCESS;
}