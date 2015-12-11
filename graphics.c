#include "graphics.h"
#include "image.h"
#include "lutro.h"
#include "compat/strl.h"
#include "retro_miscellaneous.h"

#include <stdlib.h>
#include <string.h>

static painter_t *painter;
static bitmap_t  *fbbmp;
//static uint32_t current_color;
//static uint32_t background_color;

void lutro_graphics_init()
{
   // TODO: power of two framebuffers
   painter = (painter_t*)calloc(1, sizeof(painter_t));
   lutro_graphics_reinit();

}

void lutro_graphics_reinit()
{
   if (fbbmp && fbbmp->width == settings.width && fbbmp->height == settings.height)
      return;

   if (fbbmp)
      free(fbbmp->data);
   else
      fbbmp = (bitmap_t*)calloc(1, sizeof(bitmap_t));

   settings.pitch_pixels = settings.width;
   settings.pitch        = settings.pitch_pixels * sizeof(uint32_t);
   settings.framebuffer  = (uint32_t*)calloc(1, settings.pitch * settings.height);

   fbbmp->data   = settings.framebuffer;
   fbbmp->height = settings.height;
   fbbmp->width  = settings.width;
   fbbmp->pitch  = settings.pitch;

   painter->target = fbbmp;
   pntr_reset(painter);
}

void lutro_graphics_begin_frame(lua_State *L)
{
   pntr_clear(painter);
}

void lutro_graphics_end_frame(lua_State *L)
{
   pntr_origin(painter, true);
}

static int img_getData(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");

   lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
   return 1;
}

static int img_getWidth(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->data->width);
   return 1;
}

static int img_getHeight(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->data->height);
   return 1;
}

static int img_getDimensions(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");
   lua_pushnumber(L, self->data->width);
   lua_pushnumber(L, self->data->height);
   return 2;
}

static int img_setFilter(lua_State *L)
{
   return 0;
}

static int img_gc(lua_State *L)
{
   gfx_Image* self = (gfx_Image*)luaL_checkudata(L, 1, "Image");

   if (self->ref != LUA_NOREF )
   {
      luaL_unref( L, LUA_REGISTRYINDEX, self->ref );
   }

   /* FIXME */
   /* free((void*)self); */

   return 0;
}


static int gfx_newImage(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.newImage requires 1 arguments, %d given.", n);

   gfx_Image *self = (gfx_Image*)lua_newuserdata(L, sizeof(gfx_Image));;

   if (!self)
      return 0;

   if (lua_isuserdata(L, 1))
   {
      self->data = (bitmap_t*)luaL_checkudata(L, 1, "ImageData");

      lua_pushvalue(L, 1);
      self->ref = luaL_ref(L, LUA_REGISTRYINDEX);
   }
   else
   {
      const char* path = luaL_checkstring(L, 1);
      self->data = (bitmap_t*)image_data_create(L, path);
      self->ref = luaL_ref(L, LUA_REGISTRYINDEX);
   }

   self->data->pitch = self->data->width << 2;

   if (luaL_newmetatable(L, "Image") != 0)
   {
      static luaL_Reg img_funcs[] = {
         { "getData",       img_getData },
         { "getWidth",      img_getWidth },
         { "getHeight",     img_getHeight },
         { "getDimensions", img_getDimensions },
         { "setFilter",     img_setFilter },
         { "__gc",          img_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction( L, img_gc );
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, img_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return 1;
}

static int quad_type(lua_State *L)
{
   gfx_Quad* self = (gfx_Quad*)luaL_checkudata(L, 1, "Quad");
   (void) self;
   lua_pushstring(L, "Quad");
   return 1;
}

static int quad_setViewport(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 5)
      return luaL_error(L, "Quad:setViewport requires 5 arguments, %d given.", n);

   gfx_Quad* self = (gfx_Quad*)luaL_checkudata(L, 1, "Quad");
   self->x = luaL_checknumber(L, 2);
   self->y = luaL_checknumber(L, 3);
   self->w = luaL_checknumber(L, 4);
   self->h = luaL_checknumber(L, 5);

   return 0;
}

static int quad_getViewport(lua_State *L)
{
   gfx_Quad* self = (gfx_Quad*)luaL_checkudata(L, 1, "Quad");
   lua_pushnumber(L, self->x);
   lua_pushnumber(L, self->y);
   lua_pushnumber(L, self->w);
   lua_pushnumber(L, self->h);
   return 4;
}

static int quad_gc(lua_State *L)
{
   gfx_Quad* self = (gfx_Quad*)luaL_checkudata(L, 1, "Quad");
   (void)self;
   return 0;
}

static int gfx_newQuad(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 6)
      return luaL_error(L, "lutro.graphics.newQuad requires 6 arguments, %d given.", n);

   gfx_Quad* self = (gfx_Quad*)lua_newuserdata(L, sizeof(gfx_Quad));
   self->x = luaL_checknumber(L, 1);
   self->y = luaL_checknumber(L, 2);
   self->w = luaL_checknumber(L, 3);
   self->h = luaL_checknumber(L, 4);
   self->sw = luaL_checknumber(L, 5);
   self->sh = luaL_checknumber(L, 6);

   if (luaL_newmetatable(L, "Quad") != 0)
   {
      static luaL_Reg quad_funcs[] = {
         { "type",        quad_type },
         { "getViewport", quad_getViewport },
         { "setViewport", quad_setViewport },
         { "__gc",        quad_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction( L, img_gc );
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, quad_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return 1;
}

static int font_type(lua_State *L)
{
   font_t* self = (font_t*)luaL_checkudata(L, 1, "Font");
   (void) self;
   lua_pushstring(L, "Font");
   return 1;
}

static int font_getWidth(lua_State *L)
{
   const char* text = luaL_checkstring(L, 2);

   lua_pushnumber(L, pntr_text_width(painter, text));
   return 1;
}

static int font_setFilter(lua_State *L)
{
   return 0;
}

static int font_gc(lua_State *L)
{
   font_t* self = (font_t*)luaL_checkudata(L, 1, "Font");
   (void)self;
   return 0;
}

static void push_font(lua_State *L, font_t *font)
{
   font_t* self = (font_t*)lua_newuserdata(L, sizeof(font_t));
   memcpy(self, font, sizeof(font_t));

   if (luaL_newmetatable(L, "Font") != 0)
   {
      static luaL_Reg font_funcs[] = {
         { "type",     font_type },
         { "getWidth", font_getWidth },
         { "setFilter",font_setFilter },
         { "__gc",     font_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);

      lua_setfield(L, -2, "__index");

      lua_pushcfunction( L, font_gc );
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, font_funcs, 0);
   }

   lua_setmetatable(L, -2);
}

static int gfx_newImageFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.newImageFont requires 2 arguments, %d given.", n);

   font_t *font;

   void *p = lua_touserdata(L, 1);
   if (p == NULL)
   {
      const char* path = luaL_checkstring(L, 1);
      const char* characters = luaL_checkstring(L, 2);

      char fullpath[PATH_MAX_LENGTH];
      strlcpy(fullpath, settings.gamedir, sizeof(fullpath));
      strlcat(fullpath, path, sizeof(fullpath));

      font = font_load_filename(fullpath, characters, 0);
   }
   else
   {
      gfx_Image* img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
      const char* characters = luaL_checkstring(L, 2);

      font = font_load_bitmap(img->data, characters, 0);
   }

   lua_pop(L, n);

   push_font(L, font);

   free(font);

   return 1;
}

static int gfx_setFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.setFont requires 1 arguments, %d given.", n);

   font_t* font = (font_t*)luaL_checkudata(L, 1, "Font");
   lua_pop(L, n);
   painter->font = font;

   return 0;
}

static int gfx_getFont(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getFont requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   push_font(L, painter->font);

   return 1;
}

static int gfx_setColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 3 && n != 4)
      return luaL_error(L, "lutro.graphics.setColor requires 1, 3 or 4 arguments, %d given.", n);

   gfx_Color c;

   if (lua_istable(L, 1))
   {
      for (int i = 1; i <= 4; i++)
         lua_rawgeti(L, 1, i);

      c.r = luaL_checkint(L, -4);
      c.g = luaL_checkint(L, -3);
      c.b = luaL_checkint(L, -2);
      c.a = luaL_optint(L, -1, 255);

      lua_pop(L, 4);
   }
   else
   {
      c.r = luaL_checkint(L, 1);
      c.g = luaL_checkint(L, 2);
      c.b = luaL_checkint(L, 3);
      c.a = luaL_optint(L, 4, 255);
   }

   lua_pop(L, n);

   painter->foreground = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

static int gfx_getColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getColor requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   gfx_Color c;
   c.a = (painter->foreground >> 24) & 0xff;
   c.r = (painter->foreground >> 16) & 0xff;
   c.g = (painter->foreground >>  8) & 0xff;
   c.b = (painter->foreground >>  0) & 0xff;

   lua_pushnumber(L, c.r);
   lua_pushnumber(L, c.g);
   lua_pushnumber(L, c.b);
   lua_pushnumber(L, c.a);

   return 4;
}

static int gfx_setBackgroundColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1 && n != 3 && n != 4)
      return luaL_error(L, "lutro.graphics.setBackgroundColor requires 1, 3 or 4 arguments, %d given.", n);

   gfx_Color c;

   if (lua_istable(L, 1))
   {
      for (int i = 1; i <= 4; i++)
         lua_rawgeti(L, 1, i);

      c.r = luaL_checkint(L, -4);
      c.g = luaL_checkint(L, -3);
      c.b = luaL_checkint(L, -2);
      c.a = luaL_optint(L, -1, 255);

      lua_pop(L, 4);
   }
   else
   {
      c.r = luaL_checkint(L, 1);
      c.g = luaL_checkint(L, 2);
      c.b = luaL_checkint(L, 3);
      c.a = luaL_optint(L, 4, 255);
   }

   lua_pop(L, n);

//   background_color = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;
   painter->background = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

static int gfx_getBackgroundColor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getBackgroundColor requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   gfx_Color c;
   c.a = (painter->background >> 24) & 0xff;
   c.r = (painter->background >> 16) & 0xff;
   c.g = (painter->background >>  8) & 0xff;
   c.b = (painter->background >>  0) & 0xff;

   lua_pushnumber(L, c.r);
   lua_pushnumber(L, c.g);
   lua_pushnumber(L, c.b);
   lua_pushnumber(L, c.a);

   return 4;
}

static int gfx_clear(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.clear requires 0 arguments, %d given.", n);

   lua_pop(L, n);

   pntr_clear(painter);

   return 0;
}

static int gfx_rectangle(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 5)
      return luaL_error(L, "lutro.graphics.rectangle requires 5 arguments, %d given.", n);

   const char* mode = luaL_checkstring(L, 1);
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   int w = luaL_checknumber(L, 4);
   int h = luaL_checknumber(L, 5);

   lua_pop(L, n);

   rect_t r  = { x, y, w, h };
   if (!strcmp(mode, "fill"))
   {
      pntr_fill_rect(painter, &r);
   }
   else if (!strcmp(mode, "line"))
   {
      pntr_strike_rect(painter, &r);
   }
   else
   {
      return luaL_error(L, "lutro.graphics.rectangle's available modes are : fill or line", n);
   }

   return 0;
}

static int gfx_point(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.point requires 2 arguments, %d given.", n);

   int x = luaL_checknumber(L, 1);
   int y = luaL_checknumber(L, 2);

   lua_pop(L, n);

   if (x > painter->target->width || x < 0 || y > painter->target->height || y < 0)
      return 0;

   painter->target->data[y * (painter->target->pitch >> 2) + x] = painter->foreground;

   return 0;
}

static int gfx_line(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 4)
      return luaL_error(L, "lutro.graphics.line requires 4 arguments, %d given.", n);

   int x1 = luaL_checknumber(L, 1);
   int y1 = luaL_checknumber(L, 2);
   int x2 = luaL_checknumber(L, 3);
   int y2 = luaL_checknumber(L, 4);

   lua_pop(L, n);

   int pitch_pixels = settings.pitch_pixels;
   uint32_t *framebuffer = settings.framebuffer;

   int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
   int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1;
   int err = (dx>dy ? dx : -dy)/2, e2;

   for (;;) {
      if (y1 >= 0 && y1 < settings.height)
         if (x1 >= 0 && x1 < settings.width)
            framebuffer[y1 * pitch_pixels + x1] = painter->foreground;
      if (x1==x2 && y1==y2) break;
      e2 = err;
      if (e2 >-dx) { err -= dy; x1 += sx; }
      if (e2 < dy) { err += dx; y1 += sy; }
   }

   return 0;
}

static int gfx_draw(lua_State *L)
{
   int n = lua_gettop(L);

   if (n < 1)
      return luaL_error(L, "lutro.graphics.draw requires at least 1 arguments, %d given.", n);

   int start = 0;
   gfx_Image* img = NULL;
   gfx_Quad* quad = NULL;

   void *p = lua_touserdata(L, 2);
   if (p == NULL)
   {
      img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
      start = 1;
   }
   else
   {
      img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
      quad = (gfx_Quad*)luaL_checkudata(L, 2, "Quad");
      start = 2;
   }

   int x = luaL_optnumber(L, start + 1, 0);
   int y = luaL_optnumber(L, start + 2, 0);
   float r = luaL_optnumber(L, start + 3, 0);
   float sx = luaL_optnumber(L, start + 4, 1);
   float sy = luaL_optnumber(L, start + 5, sx);
   int ox = luaL_optnumber(L, start + 6, 0);
   int oy = luaL_optnumber(L, start + 7, 0);
   int kx = luaL_optnumber(L, start + 8, 0);
   int ky = luaL_optnumber(L, start + 9, 0);

   lua_pop(L, n);

   rect_t drect = {
      x + ox,
      y + oy,
      (int)img->data->width,
      (int)img->data->width
   };

   rect_t srect = {
      0, 0,
      (int)img->data->width,
      (int)img->data->width
   };

   pntr_push(painter);
   pntr_rotate(painter, r);
   pntr_scale(painter, sx, sy);
   pntr_rotate(painter, r);

   if (quad != NULL)
   {
      srect.x = quad->x;
      srect.y = quad->y;
      srect.width = quad->w;
      srect.height = quad->h;

      drect.width = quad->w;
      drect.height = quad->h;
   }
   pntr_draw(painter, img->data, &srect, &drect);

   pntr_pop(painter);

   return 0;
}

static int gfx_print(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 3)
      return luaL_error(L, "lutro.graphics.print requires 3 arguments, %d given.", n);

   if (painter->font == NULL)
      return luaL_error(L, "lutro.graphics.print requires a font to be set.");


   const char* message = luaL_checkstring(L, 1);
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);

   pntr_print(painter, dest_x, dest_y, message);

   lua_pop(L, n);

   return 0;
}

static int gfx_printf(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 5)
      return luaL_error(L, "lutro.graphics.printf requires 5 arguments, %d given.", n);

   if (painter->font == NULL)
      return luaL_error(L, "lutro.graphics.printf requires a font to be set.");

   const char* message = luaL_checkstring(L, 1);
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);
   int limit  = luaL_checknumber(L, 4);
   const char* align = luaL_checkstring(L, 5);

   if (!strcmp(align, "right"))
      pntr_print(painter, dest_x + limit - pntr_text_width(painter, message), dest_y, message);
   else if (!strcmp(align, "center"))
      pntr_print(painter, dest_x + limit/2 - pntr_text_width(painter, message)/2, dest_y, message);
   else
      pntr_print(painter, dest_x, dest_y, message);

   lua_pop(L, n);

   return 0;
}

static int gfx_setDefaultFilter(lua_State *L)
{
   return 0;
}

static int gfx_setLineStyle(lua_State *L)
{
   return 0;
}

static int gfx_setLineWidth(lua_State *L)
{
   return 0;
}

static int gfx_scale(lua_State *L)
{
   int n = lua_gettop(L);

   if (n < 1)
      return luaL_error(L, "lutro.graphics.scale requires  at least 1 argument 0 given.");

   float x = luaL_checknumber(L, 1);
   float y = luaL_optnumber(L, 2, x);

   pntr_scale(painter, x, y);

   lua_pop(L, n);

   return 0;
}

static int gfx_rotate(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 1)
      return luaL_error(L, "lutro.graphics.rotate requires 1 arguments, %d given.", n);

   float rad = luaL_checknumber(L, 1);

   pntr_rotate(painter, rad);

   lua_pop(L, n);

   return 0;
}

static int gfx_getWidth(lua_State *L)
{
   lua_pushnumber(L, settings.width);
   return 1;
}

static int gfx_getHeight(lua_State *L)
{
   lua_pushnumber(L, settings.height);
   return 1;
}

static int gfx_origin(lua_State *L)
{
   lua_pop(L, lua_gettop(L));

   pntr_origin(painter, false);

   return 0;
}

static int gfx_pop(lua_State *L)
{
   lua_pop(L, lua_gettop(L));

   if (!pntr_pop(painter))
      return luaL_error(L, "Transformation stack underflow.");

   return 0;
}

static int gfx_push(lua_State *L)
{
   lua_pop(L, lua_gettop(L));

   if (!pntr_push(painter))
      return luaL_error(L, "Transformation stack overflow.");

   return 0;
}

static int gfx_translate(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.translate requires 2 arguments, %d given.", n);

   pntr_translate(painter, luaL_checknumber(L, 1), luaL_checknumber(L, 2));

   lua_pop(L, n);

   return 0;
}

static int gfx_setScissor(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0 && n != 4)
      return luaL_error(L, "lutro.graphics.setScissor requires 0 or 4 arguments, %d given.", n);

   rect_t r = {
      0, 0, painter->target->width, painter->target->height
   };

   if (n > 0)
   {
      r.x = luaL_checknumber(L, 1);
      r.y = luaL_checknumber(L, 2);
      r.width  = luaL_checknumber(L, 3);
      r.height = luaL_checknumber(L, 4);

      lua_pop(L, n);
   }

   painter->clip = r;
   pntr_sanitize_clip(painter);

   return 0;
}


int lutro_graphics_preload(lua_State *L)
{
   static luaL_Reg gfx_funcs[] =  {
      { "clear",        gfx_clear },
      { "draw",         gfx_draw },
      { "getBackgroundColor", gfx_getBackgroundColor },
      { "getColor",     gfx_getColor },
      { "getFont",      gfx_getFont },
      { "getHeight",    gfx_getHeight },
      { "getWidth",     gfx_getWidth },
      { "line",         gfx_line },
      { "newImage",     gfx_newImage },
      { "newImageFont", gfx_newImageFont },
      { "newQuad",      gfx_newQuad },
      { "point",        gfx_point },
      { "print",        gfx_print },
      { "printf",       gfx_printf },

      { "origin",       gfx_origin },
      { "pop",          gfx_pop },
      { "push",         gfx_push },
      { "rotate",       gfx_rotate },
      { "scale",        gfx_scale },
      { "shear",        l_not_implemented },
      { "translate",    gfx_translate },

      { "rectangle",    gfx_rectangle },
      { "setBackgroundColor", gfx_setBackgroundColor },
      { "setColor",     gfx_setColor },
      { "setDefaultFilter", gfx_setDefaultFilter },
      { "setFont",      gfx_setFont },
      { "setLineStyle", gfx_setLineStyle },
      { "setLineWidth", gfx_setLineWidth },
      { "setScissor",   gfx_setScissor },
      {NULL, NULL}
   };

   lutro_ensure_global_table(L, "lutro");

   luaL_newlib(L, gfx_funcs);

   lua_setfield(L, -2, "graphics");

   return 1;
}
