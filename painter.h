#ifndef PAINTER_H
#define PAINTER_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
   uint32_t *data;
   unsigned width, height;
   size_t pitch;
} bitmap_t;

typedef struct
{
   bitmap_t image;
   int  separators[256];
   char characters[256];
} font_t;

typedef struct
{
   int x, y, width, height;
} rect_t;

typedef struct painter_s painter_t;

struct painter_s {
   uint32_t foreground;
   uint32_t background;

   bitmap_t target;
   font_t   font;
   rect_t   clip;

   painter_t *parent;
};

painter_t *pntr_push(painter_t *p);
painter_t *pntr_pop(painter_t *p);
void pntr_reset(painter_t *p);
void pntr_clear(painter_t *p);
void pntr_sanitize_clip(painter_t *p);
void pntr_strike_rect(painter_t *p, const rect_t *rect);
void pntr_fill_rect(painter_t *p, const rect_t *rect);

rect_t rect_intersect(const rect_t *a, const rect_t *b);
int rect_is_null(const rect_t *r);

#endif // PAINTER_H
