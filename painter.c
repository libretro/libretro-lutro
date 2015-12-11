#define _GNU_SOURCE

#include "painter.h"
#include "formats/rpng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <retro_miscellaneous.h>

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


#ifdef HAVE_COMPOSITION
/* from http://www.codeguru.com/cpp/cpp/algorithms/general/article.php/c15989/Tip-An-Optimized-Formula-for-Alpha-Blending-Pixels.htm */
#define COMPOSE_FAST(S, D, A) (((S * A) + (D * (255U - A))) >> 8U)
#define DISASSEMBLE_RGB(COLOR, R, G, B) \
   R = ((COLOR & 0xff0000)>>16);\
   G = ((COLOR & 0xff00)>>8);\
   B = (COLOR & 0xff);
#endif

static int strpos(const char *haystack, char needle)
{
   char *p = strchr(haystack, needle);
   if (p)
      return p - haystack;
   return -1;
}

void pntr_reset(painter_t *p)
{
   p->background = 0xff000000;
   p->foreground = 0xffffffff;

   p->clip.x = 0;
   p->clip.y = 0;
   p->clip.width  = p->target->width;
   p->clip.height = p->target->height;

   pntr_origin(p, true);
}


void pntr_clear(painter_t *p)
{
   uint32_t *begin = p->target->data;
   uint32_t *end   = p->target->data + p->target->height * (p->target->pitch >> 2);
   uint32_t color  = p->background;

   while (begin < end)
      *begin++ = color;
}


// call this after setting the clip.
void pntr_sanitize_clip(painter_t *p)
{
   rect_t target_rect = {
      0, 0, p->target->width, p->target->height
   };

   p->clip = rect_intersect(&p->clip,  &target_rect);
}

void pntr_strike_rect(painter_t *p, const rect_t *rect)
{
//   int x1, y1, x2, y2, i;
//   uint32_t *fb = p->target->data + (p->target->pitch >> 2)

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
   size_t row_size = p->target->pitch >> 2;
   uint32_t color = p->foreground;
   rect_t drect = {
      rect->x + p->trans->tx, rect->y + p->trans->tx,
      rect->width, rect->height
   };

   drect = rect_intersect(&p->clip, &drect);

   if (rect_is_null(&drect))
      return;

   int x;
   int xend = drect.x + drect.width;
   uint32_t *row  = p->target->data + drect.y * row_size;
   uint32_t *end  = row + row_size * drect.height;

   if ((color & 0xff000000) == 0)
      return;

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
   rect_t c = { left, top, max(width, 0), max(height, 0) };

   return c;
}


int rect_is_null(const rect_t *r)
{
   return r->width <= 0 || r->height <= 0;
}


void pntr_draw(painter_t *p, const bitmap_t *bmp, const rect_t *src_rect, const rect_t *dst_rect)
{
   rect_t srect = *src_rect, drect = *dst_rect;

   drect.x += p->trans->tx;
   drect.y += p->trans->ty;

   /* scaling not supported */
   drect.width  = srect.width;
   drect.height = srect.height;

   if (drect.x < 0)
   {
      srect.x     += -drect.x;
      srect.width += drect.x;
   }

   if (drect.y < 0)
   {
      srect.y     += -drect.y;
      srect.height += drect.y;
   }

   drect        = rect_intersect(&drect, &p->clip);
   drect.width  = min(drect.width, srect.width);
   drect.height = min(drect.height, srect.height);

   if (rect_is_null(&drect) || rect_is_null(&srect))
      return;

   size_t dst_skip = p->target->pitch >> 2;
   size_t src_skip = bmp->pitch >> 2;

   uint32_t *dst = p->target->data + dst_skip * drect.y + drect.x;
   uint32_t *src = bmp->data + src_skip * srect.y + srect.x;

   int rows_left = drect.height;
   int cols = drect.width;
   int x = 0;

#ifdef HAVE_COMPOSITION
   uint32_t sa, sr, sg, sb, da, dr, dg, db, s, d;
   while (rows_left--)
   {
      for (x = 0; x < cols; ++x)
      {
         s = src[x];
         d = dst[x];
         sa = s >> 24;
         da = d >> 24;
         DISASSEMBLE_RGB(s, sr, sg, sb);
         DISASSEMBLE_RGB(d, dr, dg, db);
         dst[x] = ((sa + da * (255 - sa)) << 24) | (COMPOSE_FAST(sr, dr, sa) << 16) | (COMPOSE_FAST(sg, dg, sa) << 8) | (COMPOSE_FAST(sb, db, sa));
      }

      dst += dst_skip;
      src += src_skip;
   }
#else
   uint32_t c;
   while (rows_left--)
   {
      for (x = 0; x < cols; ++x)
      {
         c = src[x];
         if (c & 0xff000000)
            dst[x] = c;
      }

      dst += dst_skip;
      src += src_skip;
   }
#endif
}


void pntr_print(painter_t *p, int x, int y, const char *text)
{
   assert(p->font != NULL);

   if (p->font->flags & FONT_FREETYPE)
   {

   }
   else
   {
      bitmap_t *atlas = &p->font->atlas;
      font_t *font = p->font;

      rect_t drect = {
         x, y, p->target->width, atlas->height
      };

      rect_t srect = {
         0, 0, 0, atlas->height
      };

      while (*text)
      {
         int c = *text++;
         int pos = strpos(font->characters, c);

         srect.x = font->separators[pos] + 1;
         drect.width = srect.width = font->separators[pos+1] - srect.x;

         pntr_draw(p, atlas, &srect, &drect);

         drect.x += srect.width + 1;
      }
   }
}


int pntr_text_width(painter_t *p, const char *text)
{
   int width = 0;
   assert(p->font != NULL);

   if (p->font->flags & FONT_FREETYPE)
   {

   }
   else
   {
      font_t *font = p->font;

      int glyph_x, glyph_width;

      while (*text)
      {
         int c = *text++;
         int pos = strpos(font->characters, c);

         glyph_x = font->separators[pos] + 1;
         glyph_width = font->separators[pos+1] - glyph_x;

         width += glyph_width + 1;
      }
   }

   return width;
}


void pntr_printf(painter_t *p, int x, int y, const char *format, ...)
{
   char *buf;
   va_list v;
   va_start(v, format);
   vasprintf(&buf,  format, v);
   va_end(v);
   pntr_print(p, x, y, buf);
   free(buf);
}


bool pntr_push(painter_t *p)
{
   if (p->stack_pos == ARRAY_SIZE(p->stack))
      return false;

   memcpy(&p->stack[p->stack_pos + 1], &p->stack[p->stack_pos], sizeof(p->stack[0]));
   p->trans = &p->stack[++p->stack_pos];

   return true;
}


bool pntr_pop(painter_t *p)
{
   if (p->stack_pos == 0)
      return false;

   p->trans = &p->stack[--p->stack_pos];

   return true;
}


void pntr_origin(painter_t *p, bool reset_stack)
{
   if (reset_stack)
   {
      p->stack_pos = 0;
      p->trans = &p->stack[p->stack_pos];
   }

   pntr_scale(p, 1, 1);
   pntr_rotate(p, 0);
   pntr_translate(p, 0, 0);
}

void pntr_scale(painter_t *p, float x, float y)
{
   p->trans->sx = x;
   p->trans->sy = y;
}

void pntr_rotate(painter_t *p, float rad)
{
   p->trans->r = rad;
}

void pntr_translate(painter_t *p, int x, int y)
{
   p->trans->tx = x;
   p->trans->ty = y;
}

font_t *font_load_filename(const char *filename, const char *characters, unsigned flags)
{
   font_t *font = calloc(1, sizeof(font_t));

   flags &= ~FONT_FREETYPE;

   if (font->atlas.data)
      free(font->atlas.data);

   font->pxsize = 0;
   font->flags  = flags;

   bitmap_t *atlas = &font->atlas;

   rpng_load_image_argb(filename, &atlas->data, &atlas->width, &atlas->height);
   atlas->pitch = atlas->width << 2;

   uint32_t separator = atlas->data[0];
   int max_separators = 256;

   int i, char_counter = 0;
   for (i = 0; i < atlas->width && char_counter < max_separators; i++)
   {
      if (separator == atlas->data[i])
         font->separators[char_counter++] = i;
   }

   strcpy(font->characters, characters);

   return font;
}

font_t *font_load_bitmap(const bitmap_t *atlas, const char *characters, unsigned flags)
{
   font_t *font = calloc(1, sizeof(font_t));

   flags &= ~FONT_FREETYPE;

   if (font->atlas.data)
      free(font->atlas.data);

   font->pxsize = 0;
   font->flags  = flags;

   font->atlas = *atlas;

   uint32_t separator = font->atlas.data[0];
   int max_separators = 256;

   int i, char_counter = 0;
   for (i = 0; i < font->atlas.width && char_counter < max_separators; i++)
   {
      if (separator == font->atlas.data[i])
         font->separators[char_counter++] = i;
   }

   strcpy(font->characters, characters);

   return font;
}
