#include "graphics.h"
#include "image.h"
#include "lutro.h"
#include <compat/strl.h>
#include <retro_miscellaneous.h>

#include <stdlib.h>
#include <string.h>

static int def_canv = LUA_NOREF;
static int cur_canv = LUA_NOREF;
static bitmap_t  *fbbmp;
//static uint32_t current_color;
//static uint32_t background_color;

static void set_ref(lua_State *L, int *ref)
{
   if (*ref != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, *ref);

   *ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

static int canvas_type(lua_State *L);
static int canvas_gc(lua_State *L);

static int canvas_setFilter(lua_State *L)
{
   return 0;
}

static gfx_Canvas *new_canvas(lua_State *L)
{
   gfx_Canvas* self = (gfx_Canvas*)lua_newuserdata(L, sizeof(gfx_Canvas));
   memset(self, 0, sizeof(*self));

   if (luaL_newmetatable(L, "Canvas") != 0)
   {
      static luaL_Reg canvas_funcs[] = {
         { "type",      canvas_type },
         { "setFilter", canvas_setFilter },
         { "__gc",      canvas_gc },
         {NULL, NULL}
      };

      lua_pushvalue(L, -1);
      lua_setfield(L, -2, "__index");
      luaL_setfuncs(L, canvas_funcs, 0);
   }

   lua_setmetatable(L, -2);
   return self;
}

static gfx_Canvas *get_canvas_ndx(lua_State *L, int ndx)
{
   return (gfx_Canvas*)luaL_checkudata(L, ndx, "Canvas");
}

static gfx_Canvas *get_canvas_ref(lua_State *L, int ref)
{
   lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
   return get_canvas_ndx(L, -1);
}

void lutro_graphics_init(lua_State *L)
{
   // TODO: power of two framebuffers
   new_canvas(L);
   lua_pushvalue(L, -1);
   set_ref(L, &def_canv);
   set_ref(L, &cur_canv);

   lutro_graphics_reinit(L);
}

void lutro_graphics_reinit(lua_State *L)
{
   gfx_Canvas *canvas;

   if (!(fbbmp && fbbmp->width == settings.width && fbbmp->height == settings.height)) {
       if (fbbmp)
           lutro_free(fbbmp->data);
       else
           fbbmp = (bitmap_t*)lutro_calloc(1, sizeof(bitmap_t));

       settings.pitch_pixels = settings.width;
       settings.pitch        = settings.pitch_pixels * sizeof(uint32_t);
       settings.framebuffer  = (uint32_t*)lutro_calloc(1, settings.pitch * settings.height);

       fbbmp->data   = settings.framebuffer;
       fbbmp->height = settings.height;
       fbbmp->width  = settings.width;
       fbbmp->pitch  = settings.pitch;
   }

   canvas = (gfx_Canvas*)get_canvas_ref(L, cur_canv);
   canvas->target = fbbmp;
   pntr_reset(canvas);
   lua_pop(L, 1);
}

void lutro_graphics_begin_frame(lua_State *L)
{
   gfx_Canvas* canvas = get_canvas_ref(L, cur_canv);
   pntr_clear(canvas);
   lua_pop(L, 1);
}

void lutro_graphics_end_frame(lua_State *L)
{
   gfx_Canvas* canvas = get_canvas_ref(L, cur_canv);
   pntr_origin(canvas, true);
   lua_pop(L, 1);
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
   /* lutro_free((void*)self); */

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
      self->data = (bitmap_t*)image_data_create_from_path(L, path);
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

      lua_pushcfunction( L, quad_gc );
      lua_setfield( L, -2, "__gc" );

      luaL_setfuncs(L, quad_funcs, 0);
   }

   lua_setmetatable(L, -2);

   return 1;
}

static int canvas_type(lua_State *L)
{
   gfx_Canvas* self = get_canvas_ndx(L, 1);
   (void) self;
   lua_pushstring(L, "Canvas");
   return 1;
}

static int canvas_gc(lua_State *L)
{
   gfx_Canvas* self = get_canvas_ndx(L, 1);
   if (self->target) {
       if (self->target->data) {
           lutro_free(self->target->data);
           self->target->data = NULL;
       }
       lutro_free(self->target);
       self->target = NULL;
   }
   return 0;
}

static int gfx_newCanvas(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 2)
      return luaL_error(L, "lutro.graphics.newCanvas requires 2 arguments, %d given.", n);

   int w = luaL_checknumber(L, 1);
   int h = luaL_checknumber(L, 2);

   gfx_Canvas* canvas = new_canvas(L);

   bitmap_t* bmp = (bitmap_t*)lutro_calloc(1, sizeof(bitmap_t));

   int pitch = w * sizeof(uint32_t);
   uint32_t *framebuffer  = (uint32_t*)lutro_calloc(1, pitch * h);

   bmp->data   = framebuffer;
   bmp->height = h;
   bmp->width  = w;
   bmp->pitch  = pitch;

   canvas->target = bmp;
   pntr_reset(canvas);
   return 1;
}

static int gfx_setCanvas(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0 && n != 1)
      return luaL_error(L, "lutro.graphics.setCanvas requires 0 or 1 argument, %d given.", n);

   if (n == 0)
   {
      lua_rawgeti(L, LUA_REGISTRYINDEX, def_canv);
      set_ref(L, &cur_canv);
   }
   else if (n == 1)
   {
      get_canvas_ndx(L, 1);
      lua_pushvalue(L, 1);
      set_ref(L, &cur_canv);
   }

   return 0;
}

static int gfx_getCanvas(lua_State *L)
{
   int n = lua_gettop(L);

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getCanvas requires 0 arguments, %d given.", n);

   get_canvas_ref(L, cur_canv);

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
   gfx_Canvas *canvas = get_canvas_ref(L, cur_canv);
   lua_pushnumber(L, pntr_text_width(canvas, text));
   return 1;
}

static int font_setFilter(lua_State *L)
{
   return 0;
}

static int font_gc(lua_State *L)
{
   font_t* self = (font_t*)luaL_checkudata(L, 1, "Font");
   // Release both atlas/owner data when owner reaches 0
   if (self) {
       if (self->owner) {
           *self->owner = *self->owner - 1;
           if (*self->owner == 0) {
               if (self->atlas.data) {
                   lutro_free(self->atlas.data);
                   self->atlas.data = NULL;
               }
               lutro_free(self->owner);
               self->owner = NULL;
           }
       }
   }
   return 0;
}

static void push_font(lua_State *L, font_t *font)
{
   font_t* self = (font_t*)lua_newuserdata(L, sizeof(font_t));
   memcpy(self, font, sizeof(font_t));
   if (self->owner) // Increment the number of owner
       *self->owner = *self->owner + 1;

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

   if (n < 2)
      return luaL_error(L, "lutro.graphics.newImageFont requires at least 2 arguments, %d given.", n);

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

   if (n >= 3) {
       font->extraspacing = luaL_checkint(L, 3);
   } else {
       // FIXME: it should be 0 here, but 1 keep lutro compatibility
       font->extraspacing = 1;
   }

   push_font(L, font);

   // The C font object was shallow-copied in a lua object. C font object must be
   // freed here but pointer inside the object (like font->atlas) must be freed
   // in lua garbage collector (aka font_gc)
   lutro_free(font);

   return 1;
}

static int gfx_setFont(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 1)
      return luaL_error(L, "lutro.graphics.setFont requires 1 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);
   font_t* font = (font_t*)luaL_checkudata(L, 1, "Font");
   canvas->font = font;

   return 0;
}

static int gfx_getFont(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getFont requires 0 arguments, %d given.", n);


   canvas = get_canvas_ref(L, cur_canv);
   if (canvas->font == NULL) {
      // In theory, it must automatically create a default font for love 0.9.0
      // Let's just return an error as not supported
      return luaL_error(L, "lutro.graphics.getFont without user defined font isn't supported.");
   }

   push_font(L, canvas->font);

   return 1;
}

static int gfx_setColor(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;
   gfx_Color c;

   if (n != 1 && n != 3 && n != 4)
      return luaL_error(L, "lutro.graphics.setColor requires 1, 3 or 4 arguments, %d given.", n);

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

   canvas = get_canvas_ref(L, cur_canv);
   canvas->foreground = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

static int gfx_getColor(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;
   gfx_Color c;

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getColor requires 0 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);
   c.a = (canvas->foreground >> ALPHA_SHIFT) & 0xff;
   c.r = (canvas->foreground >> RED_SHIFT) & 0xff;
   c.g = (canvas->foreground >> GREEN_SHIFT) & 0xff;
   c.b = (canvas->foreground >> BLUE_SHIFT) & 0xff;

   lua_pushnumber(L, c.r);
   lua_pushnumber(L, c.g);
   lua_pushnumber(L, c.b);
   lua_pushnumber(L, c.a);

   return 4;
}

static int gfx_setBackgroundColor(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

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

   canvas = get_canvas_ref(L, cur_canv);
//   background_color = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;
   canvas->background = (c.a<<24) | (c.r<<16) | (c.g<<8) | c.b;

   return 0;
}

static int gfx_getBackgroundColor(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;
   gfx_Color c;

   if (n != 0)
      return luaL_error(L, "lutro.graphics.getBackgroundColor requires 0 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);
   c.a = (canvas->background >> ALPHA_SHIFT) & 0xff;
   c.r = (canvas->background >> RED_SHIFT) & 0xff;
   c.g = (canvas->background >> GREEN_SHIFT) & 0xff;
   c.b = (canvas->background >> BLUE_SHIFT) & 0xff;

   lua_pushnumber(L, c.r);
   lua_pushnumber(L, c.g);
   lua_pushnumber(L, c.b);
   lua_pushnumber(L, c.a);

   return 4;
}

static int gfx_clear(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 0)
      return luaL_error(L, "lutro.graphics.clear requires 0 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);
   pntr_clear(canvas);

   return 0;
}

static int gfx_rectangle(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 5)
      return luaL_error(L, "lutro.graphics.rectangle requires 5 arguments, %d given.", n);

   const char* mode = luaL_checkstring(L, 1);
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   int w = luaL_checknumber(L, 4);
   int h = luaL_checknumber(L, 5);

   canvas = get_canvas_ref(L, cur_canv);
   rect_t r  = { x, y, w, h };

   if (!strcmp(mode, "fill"))
   {
      pntr_fill_rect(canvas, &r);
   }
   else if (!strcmp(mode, "line"))
   {
      pntr_strike_rect(canvas, &r);
   }
   else
   {
      return luaL_error(L, "lutro.graphics.rectangle's available modes are : fill or line", n);
   }

   return 0;
}

static int gfx_polygon(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n == 2)
      return luaL_error(L, "lutro.graphics.polygon does not currently support drawing Polygon from a table.", n);

   if ((n % 2) != 1)
      return luaL_error(L, "lutro.graphics.polygon requires an odd number of arguments, %d given.", n);

   const char* mode = luaL_checkstring(L, 1);

   canvas = get_canvas_ref(L, cur_canv);

   int* points = lutro_calloc(n-1, sizeof(int));
   for (int i = 2; i <= n; ++i)
   {
      points[i-2] = luaL_checknumber(L, i);
   }

   if (!strcmp(mode, "fill"))
   {
      pntr_fill_poly(canvas, points, n-1);
      lutro_free(points);
   }
   else if (!strcmp(mode, "line"))
   {
      pntr_strike_poly(canvas, points, n-1);
      lutro_free(points);
   }
   else
   {
      lutro_free(points);
      return luaL_error(L, "lutro.graphics.polygon's available modes are : fill or line", n);
   }

   return 0;
}

static int gfx_circle(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if ((n != 4) && (n != 5))
      return luaL_error(L, "lutro.graphics.circle requires 4 or 5 arguments, %d given.", n);

   const char* mode = luaL_checkstring(L, 1);
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   int radius = luaL_checknumber(L, 4);
   int nb_segments = 0;
   if (n == 5)
     nb_segments = luaL_checknumber(L, 5);
   if (nb_segments <= 0)
     nb_segments = MAX(10, radius);

   canvas = get_canvas_ref(L, cur_canv);

   if (!strcmp(mode, "fill"))
   {
      pntr_fill_ellipse(canvas, x, y, radius, radius, nb_segments);
   }
   else if (!strcmp(mode, "line"))
   {
      pntr_strike_ellipse(canvas, x, y, radius, radius, nb_segments);
   }
   else
   {
      return luaL_error(L, "lutro.graphics.circle's available modes are : fill or line", n);
   }

   return 0;
}

static int gfx_ellipse(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if ((n != 5) && (n != 6))
      return luaL_error(L, "lutro.graphics.ellipse requires 5 or 6 arguments, %d given.", n);

   const char* mode = luaL_checkstring(L, 1);
   int x = luaL_checknumber(L, 2);
   int y = luaL_checknumber(L, 3);
   int x_radius = luaL_checknumber(L, 4);
   int y_radius = luaL_checknumber(L, 5);
   int nb_segments = 0;
   if (n == 6)
     nb_segments = luaL_checknumber(L, 6);
   if (nb_segments <= 0)
     nb_segments = MAX(10, MAX(x_radius, y_radius));

   canvas = get_canvas_ref(L, cur_canv);

   if (!strcmp(mode, "fill"))
   {
      pntr_fill_ellipse(canvas, x, y, x_radius, y_radius, nb_segments);
   }
   else if (!strcmp(mode, "line"))
   {
      pntr_strike_ellipse(canvas, x, y, x_radius, y_radius, nb_segments);
   }
   else
   {
      return luaL_error(L, "lutro.graphics.circle's available modes are : fill or line", n);
   }

   return 0;
}

static int gfx_point(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 2)
      return luaL_error(L, "lutro.graphics.point requires 2 arguments, %d given.", n);

   int x = luaL_checknumber(L, 1);
   int y = luaL_checknumber(L, 2);

   canvas = get_canvas_ref(L, cur_canv);

   if (x > canvas->target->width || x < 0 || y > canvas->target->height || y < 0)
      return 0;

   canvas->target->data[y * (canvas->target->pitch >> 2) + x] = canvas->foreground;

   return 0;
}

/**
 * lutro.graphics.points(x, y, ... )
 *
 * https://love2d.org/wiki/love.graphics.points
 */
static int gfx_points(lua_State *L)
{
   int n = lua_gettop(L), i, x, y;
   gfx_Canvas *canvas;

   if (n == 1) {
      // TODO: Implement drawing Point tables https://love2d.org/wiki/love.graphics.points
      return luaL_error(L, "lutro.graphics.points does not currently support drawing Point types in a table.");
   }
   if (n < 2)
      return luaL_error(L, "lutro.graphics.points requires at least 2 arguments, %d given.", n);
   if (n & 1)
      return luaL_error(L, "lutro.graphics.points requires an even amount of arguments, %d arguments given.", n);

   canvas = get_canvas_ref(L, cur_canv);

   for (i = 1; i < n; i += 2) {
      x = luaL_checknumber(L, i);
      y = luaL_checknumber(L, i + 1);

      if (x > canvas->target->width || x < 0 || y > canvas->target->height || y < 0)
         // Skip if the point is out of the canvas.
         continue;

      canvas->target->data[y * (canvas->target->pitch >> 2) + x] = canvas->foreground;
   }

   return 0;
}

static int gfx_line(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 4)
      return luaL_error(L, "lutro.graphics.line requires 4 arguments, %d given.", n);

   int x1 = luaL_checknumber(L, 1);
   int y1 = luaL_checknumber(L, 2);
   int x2 = luaL_checknumber(L, 3);
   int y2 = luaL_checknumber(L, 4);

   canvas = get_canvas_ref(L, cur_canv);

   pntr_strike_line(canvas, x1, y1, x2, y2);

   return 0;
}

/* TODO: move this elsewhere, we will certainly need it a lot */
void *checkudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get correct metatable */
      if (lua_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        lua_pop(L, 2);  /* remove both metatables */
        return p;
      }
    }
  }
  return NULL;  /* to avoid warnings */
}

#define OPTNUMBER(L, ndx, def) (lua_isnumber(L, ndx) ? lua_tonumber(L, ndx) : def)

static int gfx_draw(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n < 1)
      return luaL_error(L, "lutro.graphics.draw requires at least 1 arguments, %d given.", n);

   int start = 0;
   gfx_Image* img = NULL;
   gfx_Quad* quad = NULL;
   gfx_Canvas* cnv = NULL;
   bitmap_t* data = NULL;

   void *p = lua_touserdata(L, 2);
   if (p == NULL)
   {
      img = (gfx_Image*)checkudata(L, 1, "Image");

      if (img == NULL)
      {
         cnv = get_canvas_ndx(L, 1);
         data = cnv->target;
      }
      else
      {
         data = img->data;
      }
      start = 1;
   }
   else
   {
      img = (gfx_Image*)luaL_checkudata(L, 1, "Image");
      data = img->data;
      quad = (gfx_Quad*)luaL_checkudata(L, 2, "Quad");
      start = 2;
   }

   int x = OPTNUMBER(L, start + 1, 0);
   int y = OPTNUMBER(L, start + 2, 0);
   float r = OPTNUMBER(L, start + 3, 0);
   float sx = OPTNUMBER(L, start + 4, 1);
   float sy = OPTNUMBER(L, start + 5, sx);
   int ox = OPTNUMBER(L, start + 6, 0);
   int oy = OPTNUMBER(L, start + 7, 0);
   // TODO: Make use of the kx ky shearing numbers in lutro.graphics.draw()
   // int kx = OPTNUMBER(L, start + 8, 0);
   // int ky = OPTNUMBER(L, start + 9, 0);

   rect_t drect = {
      x + ox,
      y + oy,
      (int)data->width,
      (int)data->height
   };

   rect_t srect = {
      0, 0,
      (int)data->width,
      (int)data->height
   };

   canvas = get_canvas_ref(L, cur_canv);
   pntr_push(canvas);
   pntr_rotate(canvas, r);
   pntr_scale(canvas, sx, sy);
   pntr_rotate(canvas, r);

   if (quad != NULL)
   {
      srect.x = quad->x;
      srect.y = quad->y;
      srect.width = quad->w;
      srect.height = quad->h;

      drect.width = quad->w;
      drect.height = quad->h;
   }
   pntr_draw(canvas, data, &srect, &drect);

   pntr_pop(canvas);

   return 0;
}

static int gfx_print(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 3)
      return luaL_error(L, "lutro.graphics.print requires 3 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);

   if (canvas->font == NULL)
      return luaL_error(L, "lutro.graphics.print requires a font to be set.");

   const char* message = luaL_checkstring(L, 1);
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);

   pntr_print(canvas, dest_x, dest_y, message, 0);

   return 0;
}

static int gfx_printf(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 5)
      return luaL_error(L, "lutro.graphics.printf requires 5 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);

   if (canvas->font == NULL)
      return luaL_error(L, "lutro.graphics.printf requires a font to be set.");

   const char* message = luaL_checkstring(L, 1);
   int dest_x = luaL_checknumber(L, 2);
   int dest_y = luaL_checknumber(L, 3);
   int limit  = luaL_checknumber(L, 4);
   const char* align = luaL_checkstring(L, 5);

   if (!strcmp(align, "right"))
      pntr_print(canvas, dest_x + limit - pntr_text_width(canvas, message), dest_y, message, limit);
   else if (!strcmp(align, "center"))
      pntr_print(canvas, dest_x + limit/2 - pntr_text_width(canvas, message)/2, dest_y, message, limit);
   else
      pntr_print(canvas, dest_x, dest_y, message, limit);

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
   gfx_Canvas *canvas;

   if (n < 1)
      return luaL_error(L, "lutro.graphics.scale requires  at least 1 argument 0 given.");

   float x = luaL_checknumber(L, 1);
   float y = luaL_optnumber(L, 2, x);

   canvas = get_canvas_ref(L, cur_canv);
   pntr_scale(canvas, x, y);

   return 0;
}

static int gfx_rotate(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 1)
      return luaL_error(L, "lutro.graphics.rotate requires 1 arguments, %d given.", n);

   float rad = luaL_checknumber(L, 1);

   canvas = get_canvas_ref(L, cur_canv);
   pntr_rotate(canvas, rad);

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
   gfx_Canvas *canvas = get_canvas_ref(L, cur_canv);
   pntr_origin(canvas, false);
   return 0;
}

static int gfx_pop(lua_State *L)
{
   gfx_Canvas *canvas = get_canvas_ref(L, cur_canv);

   if (!pntr_pop(canvas))
      return luaL_error(L, "Transformation stack underflow.");

   return 0;
}

static int gfx_push(lua_State *L)
{
   gfx_Canvas *canvas = get_canvas_ref(L, cur_canv);

   if (!pntr_push(canvas))
      return luaL_error(L, "Transformation stack overflow.");

   return 0;
}

static int gfx_translate(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 2)
      return luaL_error(L, "lutro.graphics.translate requires 2 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);
   pntr_translate(canvas, luaL_checknumber(L, 1), luaL_checknumber(L, 2));

   return 0;
}

static int gfx_setScissor(lua_State *L)
{
   int n = lua_gettop(L);
   gfx_Canvas *canvas;

   if (n != 0 && n != 4)
      return luaL_error(L, "lutro.graphics.setScissor requires 0 or 4 arguments, %d given.", n);

   canvas = get_canvas_ref(L, cur_canv);

   rect_t r = {
      0, 0, canvas->target->width, canvas->target->height
   };

   if (n > 0)
   {
      r.x = luaL_checknumber(L, 1);
      r.y = luaL_checknumber(L, 2);
      r.width  = luaL_checknumber(L, 3);
      r.height = luaL_checknumber(L, 4);

      lua_pop(L, n);
   }

   canvas->clip = r;
   pntr_sanitize_clip(canvas);

   return 0;
}

/**
 * lutro.graphics.present
 *
 * https://love2d.org/wiki/love.graphics.present
 */
static int gfx_present(lua_State *L)
{
   // Since Lutro handles drawing internally, we will ignore this call.
   return 0;
}

int lutro_graphics_preload(lua_State *L)
{
   static const luaL_Reg gfx_funcs[] =  {
      { "clear",        gfx_clear },
      { "draw",         gfx_draw },
      { "getBackgroundColor", gfx_getBackgroundColor },
      { "getColor",     gfx_getColor },
      { "getFont",      gfx_getFont },
      { "getHeight",    gfx_getHeight },
      { "getWidth",     gfx_getWidth },
      { "getCanvas",    gfx_getCanvas },
      { "line",         gfx_line },
      { "newImage",     gfx_newImage },
      { "newImageFont", gfx_newImageFont },
      { "newQuad",      gfx_newQuad },
      { "newCanvas",    gfx_newCanvas },
      { "point",        gfx_point },
      { "points",       gfx_points },
      { "present",      gfx_present },
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
      { "polygon",      gfx_polygon },
      { "circle",       gfx_circle },
      { "ellipse",      gfx_ellipse },
      { "setBackgroundColor", gfx_setBackgroundColor },
      { "setColor",     gfx_setColor },
      { "setDefaultFilter", gfx_setDefaultFilter },
      { "setFont",      gfx_setFont },
      { "setLineStyle", gfx_setLineStyle },
      { "setLineWidth", gfx_setLineWidth },
      { "setScissor",   gfx_setScissor },
      { "setCanvas",    gfx_setCanvas },
      {NULL, NULL}
   };

   lutro_newlib(L, gfx_funcs, "graphics"); 

   return 1;
}
