INCLUDES=-I.
DEFINES=-DOUTSIDE_SPEEX -DRANDOM_PREFIX=speex -DEXPORT= -DFIXED_POINT

#CFLAGS=-O3 --std=c99 -Wall $(INCLUDES) $(DEFINES) -fPIC
CFLAGS=-O0 -g --std=c99 -Wall $(INCLUDES) $(DEFINES) -fPIC

OBJS=rl_backgrnd.o rl_base64.o rl_config.o rl_image.o rl_imgdata.o rl_pack.o rl_rand.o rl_resample.o rl_sound.o rl_snddata.o rl_sprite.o rl_version.o rl_xml.o external/resample.o

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

all: libretroluxury.a

libretroluxury.a: $(OBJS)
	ar rcs $@ $+

rl_version.c: FORCE
	cat ../etc/version.c.templ | sed s/HASH/`git rev-parse HEAD | tr -d "\n"`/g > $@

clean:
	rm -f libretroluxury.a $(OBJS)

.PHONY: clean FORCE
