#include "painter.h"
#include <retro_miscellaneous.h>

#include <stdlib.h>
#include <string.h>

painter_t *pntr_push(painter_t *p)
{
   painter_t *painter = calloc(1, sizeof(painter_t));
   memcpy(painter, p, sizeof(painter_t));
   painter->parent = p;

   return painter;
}


painter_t *pntr_pop(painter_t *p)
{
   painter_t *painter = p->parent;

   if (painter)
      free(p);
   else
      painter = p;

   return painter;
}


void pntr_reset(painter_t *p)
{
   p->background = 0xff000000;
   p->foreground = 0xffffffff;

   p->clip.x = 0;
   p->clip.y = 0;
   p->clip.width  = p->target.width;
   p->clip.height = p->target.height;
}


void pntr_clear(painter_t *p)
{
   uint32_t *begin = p->target.data;
   uint32_t *end   = p->target.data + p->target.height * (p->target.pitch >> 2);
   uint32_t color  = p->background;

   while (begin < end)
      *begin++ = color;
}


// call this after setting the clip.
void pntr_sanitize_clip(painter_t *p)
{
   rect_t target_rect = {
      0, 0, p->target.width, p->target.height
   };

   p->clip = rect_intersect(&p->clip,  &target_rect);
}


void pntr_strike_rect(painter_t *p, const rect_t *rect)
{
//   int x1, y1, x2, y2, i;
//   uint32_t *fb = p->target.data + (p->target.pitch >> 2)

//   x1 = rect->x;
//   y1 = rect->y;
//   x2 = x1 + rect->width;
//   y2 = y1 + rect->height;


//   for (i = y1; i < y2; i++)
//   {
//      framebuffer[i * pitch_pixels + x1] = current_color;
//      framebuffer[i * pitch_pixels + x2-1] = current_color;
//   }

//   for (i = x1; i < x2; i++)
//   {
//      framebuffer[y1 * pitch_pixels + i] = current_color;
//      framebuffer[(y1+h-1) * pitch_pixels + i] = current_color;
//   }
}


void pntr_fill_rect(painter_t *p, const rect_t *rect)
{
   size_t row_size = p->target.pitch >> 2;
   rect_t drect = rect_intersect(&p->clip, rect);
   uint32_t color = p->foreground;

   if (rect_is_null(&drect))
      return;

   int x;
   int xend = drect.x + drect.width;
   uint32_t *row  = p->target.data + drect.y * row_size;
   uint32_t *end  = row + row_size * drect.height;

   do
   {
      for (x = drect.x; x < xend; ++x)
         row[x] = color;

      row += row_size;
   } while (row < end);
}


rect_t rect_intersect(const rect_t *a, const rect_t *b)
{
   int left   = max(a->x, b->x);
   int right  = min(a->x + a->width, b->x + b->width);
   int top    = max(a->y, b->y);
   int bottom = min(a->y + a->height, b->y + b->height);
   int width  = right - left;
   int height = bottom - top;
   rect_t c = { left, top, width, height };

   return c;
}


int rect_is_null(const rect_t *r)
{
   return r->width <= 0 || r->height <= 0;
}
