#ifndef PAINTER_H
#define PAINTER_H

#include <stddef.h>
#include <stdint.h>
#include <boolean.h>

enum {
   FONT_FREETYPE = 1 << 1,
   FONT_BOLD     = 1 << 2,
   FONT_ITALICS  = 1 << 3,
   FONT_STRIKE   = 1 << 4
};

typedef struct
{
   uint32_t *data;
   unsigned width, height;
   size_t pitch;
} bitmap_t;

typedef struct
{
   bitmap_t atlas; /* atlas.data is owned by the font */
   unsigned flags;
   unsigned pxsize;

   int  separators[256];
   char characters[256];
} font_t;

typedef struct
{
   int x, y, width, height;
} rect_t;

typedef struct
{
   /* translation */
   int tx;
   int ty;
   /* rotation */
   float r;
   /* scale */
   float sx;
   float sy;
} painter_transform_t;

typedef struct painter_s painter_t;

struct painter_s
{
   uint32_t foreground;
   uint32_t background;

   bitmap_t *target;
   font_t   *font;
   rect_t   clip;

   painter_transform_t *trans;
   painter_transform_t stack[64];
   size_t stack_pos;

   painter_t *parent;
};

void pntr_reset(painter_t *p);
void pntr_clear(painter_t *p);
void pntr_sanitize_clip(painter_t *p);
void pntr_strike_rect(painter_t *p, const rect_t *rect);
void pntr_fill_rect(painter_t *p, const rect_t *rect);
void pntr_draw(painter_t *p, const bitmap_t *bmp, const rect_t *src_rect, const rect_t *dst_rect);
void pntr_print(painter_t *p, int x, int y, const char *text);
int  pntr_text_width(painter_t *p, const char *text);
void pntr_printf(painter_t *p, int x, int y, const char *format, ...);

/* Transformations */
bool pntr_push(painter_t *p);
bool pntr_pop(painter_t *p);
void pntr_origin(painter_t *p, bool reset_stack);
void pntr_scale(painter_t *p, float x, float y);
void pntr_rotate(painter_t *p, float rad);
void pntr_translate(painter_t *p, int x, int y);

font_t *font_load_filename(const char *filename, const char *characters, unsigned flags);
font_t *font_load_bitmap(const bitmap_t *bmp, const char *characters, unsigned flags);

rect_t rect_intersect(const rect_t *a, const rect_t *b);
int rect_is_null(const rect_t *r);

#endif // PAINTER_H
