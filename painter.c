#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <retro_miscellaneous.h>

#include "lutro.h"
#include "painter.h"
#include "image.h"
#include "lutro_stb_image.h"

#include "encodings/utf.h"

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef HAVE_COMPOSITION
/* from http://www.codeguru.com/cpp/cpp/algorithms/general/article.php/c15989/Tip-An-Optimized-Formula-for-Alpha-Blending-Pixels.htm */
#define COMPOSE_FAST(S, D, A) (((S * A) + (D * (256U - A))) >> 8U)
#define DISASSEMBLE_RGB(COLOR, R, G, B) \
   R = ((COLOR & RED_MASK) >> RED_SHIFT);\
   G = ((COLOR & GREEN_MASK) >> GREEN_SHIFT);\
   B = ((COLOR & BLUE_MASK) >> BLUE_SHIFT);
#endif

static int strpos(const uint32_t *haystack, uint32_t needle)
{
   // Note on performance a hash table would be much faster here
   for (int i = 0; i < MAX_FONT_CHAR; i++) {
       if (haystack[i] == 0)
           break;
       if (haystack[i] == needle)
           return i;
   }
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
   if (!p->target->data)
      return;

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

void pntr_strike_line(painter_t *p, int x1, int y1, int x2, int y2)
{
   if (!p->target->data)
      return;

   uint32_t color = p->foreground;
   if ((color & 0xff000000) == 0)
      return;

   int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
   int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1;
   int err = (dx>dy ? dx : -dy)/2, e2;

   for (;;) {
      if (y1 >= 0 && y1 < p->target->height)
         if (x1 >= 0 && x1 < p->target->width)
            p->target->data[y1 * (p->target->pitch >> 2) + x1] = color;
      if (x1==x2 && y1==y2) break;
      e2 = err;
      if (e2 >-dx) { err -= dy; x1 += sx; }
      if (e2 < dy) { err += dx; y1 += sy; }
   }
}

void pntr_strike_rect(painter_t *p, const rect_t *rect)
{
   int x1 = rect->x;
   int y1 = rect->y;
   int x2 = x1 + rect->width;
   int y2 = y1 + rect->height;

   pntr_strike_line(p, x1, y1, x1, y2);
   pntr_strike_line(p, x1, y2, x2, y2);
   pntr_strike_line(p, x2, y2, x2, y1);
   pntr_strike_line(p, x2, y1, x1, y1);
}


void pntr_fill_rect(painter_t *p, const rect_t *rect)
{
   if (!p->target->data)
      return;

   size_t row_size = p->target->pitch >> 2;
   uint32_t color = p->foreground;
   rect_t drect = {
      rect->x + p->trans->tx, rect->y + p->trans->ty,
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

#ifdef HAVE_COMPOSITION
   uint32_t sa, sr, sg, sb, da, dr, dg, db, s, d;
   do
   {
      for (x = drect.x; x < xend; ++x)
      {
         s = color;
         d = row[x];
         sa = s >> 24;
         da = d >> 24;
         DISASSEMBLE_RGB(s, sr, sg, sb);
         DISASSEMBLE_RGB(d, dr, dg, db);
         row[x] = ((sa + da * (255 - sa)) << 24) | (COMPOSE_FAST(sr, dr, sa) << 16) | (COMPOSE_FAST(sg, dg, sa) << 8) | (COMPOSE_FAST(sb, db, sa));
      }

      row += row_size;
   } while (row < end);
#else
   do
   {
      for (x = drect.x; x < xend; ++x)
         row[x] = color;

      row += row_size;
   } while (row < end);
#endif
}


rect_t rect_intersect(const rect_t *a, const rect_t *b)
{
   int left   = MAX(a->x, b->x);
   int right  = MIN(a->x + a->width, b->x + b->width);
   int top    = MAX(a->y, b->y);
   int bottom = MIN(a->y + a->height, b->y + b->height);
   int width  = right - left;
   int height = bottom - top;
   rect_t c = { left, top, MAX(width, 0), MAX(height, 0) };

   return c;
}


int rect_is_null(const rect_t *r)
{
   return r->width <= 0 || r->height <= 0;
}

void pntr_strike_poly(painter_t *p, const int *points, int nb_points)
{
   if ((nb_points % 2) != 0)
      return;

   uint32_t color = p->foreground;
   if ((color & 0xff000000) == 0)
      return;

   for (int i = 0; i < (nb_points / 2); ++i)
   {
      int x1 = points[2 * i];
      int y1 = points[(2 * i) + 1];
      int x2, y2;
      if (i < ((nb_points / 2) - 1))
      {
         x2 = points[2 * (i + 1)];
         y2 = points[(2 * (i + 1)) + 1];
      }
      else
      {
         x2 = points[0];
         y2 = points[1];
      }

      pntr_strike_line(p, x1, y1, x2, y2);
   }
}

void pntr_fill_poly(painter_t *p, const int *points, int nb_points)
{
   if (!p->target->data)
      return;

   if ((nb_points % 2) != 0)
      return;

   uint32_t color = p->foreground;
   if ((color & 0xff000000) == 0)
      return;

   // find the top-most and bottom-most points
   int ymin = p->target->height + 1;
   int ymax = -1;
   for (int i = 0; i < (nb_points / 2); ++i)
   {
     ymin = MIN(ymin, points[(2 * i) + 1]);
     ymax = MAX(ymax, points[(2 * i) + 1]);
   }

   // Note: the following algorithm is correct for convex polygons only.
   // However, the polygon fill function in Love2D is also incorrect for non-convex polygons.
   for (int yy = ymin; yy <= ymax; ++yy)
   {
      int xmin = p->target->width + 1;
      int xmax = -1;
      for (int i = 0; i < (nb_points / 2); ++i)
      {
         int x1 = points[2 * i];
         int y1 = points[(2 * i) + 1];
         int x2, y2;
         if (i < ((nb_points / 2) - 1))
         {
            x2 = points[2 * (i + 1)];
            y2 = points[(2 * (i + 1)) + 1];
         }
         else
         {
            x2 = points[0];
            y2 = points[1];
         }

         if ((y1 > yy) != (y2 > yy))
         {
            int testx = x1 + ((x2 - x1) * (yy - y1)) / (y2 - y1);
            xmin = MIN(xmin, testx);
            xmax = MAX(xmax, testx);
         }
      }

      for (int xx = xmin; xx <= xmax; ++xx)
      {
         if (yy >= 0 && yy < p->target->height)
            if (xx >= 0 && xx < p->target->width)
               p->target->data[yy * (p->target->pitch >> 2) + xx] = color;
      }
   }
}

void pntr_strike_ellipse(painter_t *p, int x, int y, int radius_x, int radius_y, int nb_segments)
{
   for (int i = 0; i < nb_segments; ++i)
   {
      int x1 = x + (radius_x * cos(2 * i * M_PI / nb_segments));
      int y1 = y + (radius_y * sin(2 * i * M_PI / nb_segments));
      int x2 = x + (radius_x * cos(2 * (i + 1) * M_PI / nb_segments));
      int y2 = y + (radius_y * sin(2 * (i + 1) * M_PI / nb_segments));
      pntr_strike_line(p, x1, y1, x2, y2);
   }
}

void pntr_fill_ellipse(painter_t *p, int x, int y, int radius_x, int radius_y, int nb_segments)
{
   if (!p->target->data)
      return;

   uint32_t color = p->foreground;
   if ((color & 0xff000000) == 0)
      return;

   for (int yy = y - radius_y; yy <= y + radius_y; ++yy)
   {
      int xmin = p->target->width + 1;
      int xmax = -1;
      for (int i = 0; i < nb_segments; ++i)
      {
         int x1 = x + (radius_x * cos(2 * i * M_PI / nb_segments));
         int y1 = y + (radius_y * sin(2 * i * M_PI / nb_segments));
         int x2 = x + (radius_x * cos(2 * (i + 1) * M_PI / nb_segments));
         int y2 = y + (radius_y * sin(2 * (i + 1) * M_PI / nb_segments));

         if ((y1 > yy) != (y2 > yy))
         {
            int testx = x1 + ((x2 - x1) * (yy - y1)) / (y2 - y1);
            xmin = MIN(xmin, testx);
            xmax = MAX(xmax, testx);
         }
      }

      for (int xx = xmin; xx <= xmax; ++xx)
      {
         if (yy >= 0 && yy < p->target->height)
            if (xx >= 0 && xx < p->target->width)
               p->target->data[yy * (p->target->pitch >> 2) + xx] = color;
      }
   }
}

void pntr_draw(painter_t *p, const bitmap_t *bmp, const rect_t *src_rect, const rect_t *dst_rect)
{
   if (!p->target->data)
      return;

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
   drect.width  = MIN(drect.width, srect.width);
   drect.height = MIN(drect.height, srect.height);

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


void pntr_print(painter_t *p, int x, int y, const char *text, int limit)
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

      size_t char_nb = utf8len(text);
      uint32_t *utf32 = NULL;
      // Avoid to call malloc for small string rendering
      uint32_t buf[256];
      if (char_nb < 256) {
          utf32 = buf;
      } else {
          utf32 = lutro_malloc(char_nb * 4);
      }
      utf8_conv_utf32(utf32, char_nb, text, strlen(text));

      for(int i = 0; i < char_nb; i++)
      {
         uint32_t c = utf32[i];
         int pos = strpos(font->characters, c);
         if (pos < 0)
             continue;

         srect.x = font->separators[pos] + 1;
         drect.width = srect.width = font->separators[pos+1] - srect.x;

         pntr_draw(p, atlas, &srect, &drect);

         drect.x += srect.width + font->extraspacing;

         if (limit > 0 && drect.x - x > limit)
         {
            drect.x = x;
            drect.y += atlas->height;
         }

         if (c == '\n')
         {
            drect.x = x;
            drect.y += atlas->height;
         }
      }

      if (utf32 != buf)
          lutro_free(utf32);
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

      size_t char_nb = utf8len(text);
      uint32_t *utf32 = NULL;
      // Avoid to call malloc for small string rendering
      uint32_t buf[256];
      if (char_nb < 256) {
          utf32 = buf;
      } else {
          utf32 = lutro_malloc(char_nb * 4);
      }
      utf8_conv_utf32(utf32, char_nb, text, strlen(text));

      for(int i = 0; i < char_nb; i++)
      {
         uint32_t c = utf32[i];
         int pos = strpos(font->characters, c);
         if (pos < 0)
             continue;

         glyph_x = font->separators[pos] + 1;
         glyph_width = font->separators[pos+1] - glyph_x;

         width += glyph_width + font->extraspacing;
      }

      if (utf32 != buf)
          lutro_free(utf32);
   }

   return width;
}

#if defined(__QNX__) || defined(_MSC_VER)
// declared in libretro.c for QNX and MSC but not exposed via header file. prototype is missing.
extern int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

void pntr_printf(painter_t *p, int x, int y, const char *format, ...)
{
   char *buf;
   va_list v;
   va_start(v, format);
   vasprintf(&buf,  format, v);
   va_end(v);
   pntr_print(p, x, y, buf, 0);
   lutro_free(buf);
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
   p->trans->tx += x;
   p->trans->ty += y;
}

font_t *font_load_filename(const char *filename, const char *characters, unsigned flags)
{
   // FIXME: note it would be better to refactor/rewrite this function as
   // However special care must be taken for allocation/free outside of those functions
   // 1/ create a bitmap from filename
   // 2/ return font_load_bitmap(bitmap, characters, flags)

   font_t *font = lutro_calloc(1, sizeof(font_t));
   font->owner = lutro_calloc(1, sizeof(uint32_t));

   flags &= ~FONT_FREETYPE;

   font->pxsize = 0;
   font->flags  = flags;

   bitmap_t *atlas = &font->atlas;

   lutro_stb_image_load(filename, &atlas->data, &atlas->width, &atlas->height);
   atlas->pitch = atlas->width << 2;

   uint32_t separator = atlas->data[0];
   int max_separators = MAX_FONT_CHAR;

   int i, char_counter = 0;
   for (i = 0; i < atlas->width && char_counter < max_separators; i++)
   {
      if (separator == atlas->data[i])
         font->separators[char_counter++] = i;
   }

   // Note: later 'len' could be used to dynamically alloc the font->characters and
   // font->separators buffers
   size_t len = utf8len(characters);
   if (len > MAX_FONT_CHAR) {
       fprintf(stderr, "Font atlas is too big. It will be truncated !\n");
   }
   utf8_conv_utf32(font->characters, MAX_FONT_CHAR, characters, strlen(characters));

   return font;
}

font_t *font_load_bitmap(const bitmap_t *atlas, const char *characters, unsigned flags)
{
   font_t *font = lutro_calloc(1, sizeof(font_t));
   font->owner = lutro_calloc(1, sizeof(uint32_t));

   // Deep copy data from atlas to give ownership. It also matches the behavior
   // of font_load_filename that allocate a buffer for the atlas
   font->atlas = *atlas;
   font->atlas.data = lutro_malloc(atlas->pitch * atlas->height);
   memcpy(font->atlas.data, atlas->data, atlas->pitch * atlas->height);

   flags &= ~FONT_FREETYPE;

   font->pxsize = 0;
   font->flags  = flags;

   uint32_t separator = font->atlas.data[0];
   int max_separators = MAX_FONT_CHAR;

   int i, char_counter = 0;
   for (i = 0; i < font->atlas.width && char_counter < max_separators; i++)
   {
      if (separator == font->atlas.data[i])
         font->separators[char_counter++] = i;
   }

   // Note: later 'len' could be used to dynamically alloc the font->characters and
   // font->separators buffers
   size_t len = utf8len(characters);
   if (len > MAX_FONT_CHAR) {
       fprintf(stderr, "Font atlas is too big. It will be truncated !\n");
   }
   utf8_conv_utf32(font->characters, MAX_FONT_CHAR, characters, strlen(characters));

   return font;
}
