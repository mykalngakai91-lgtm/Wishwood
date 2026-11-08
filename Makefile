CC = clang
INCLUDES = -I$(OO_PS4_TOOLCHAIN)/include
SDLINC = -I$(OO_PS4_TOOLCHAIN)/include/SDL2
SRCDIR = -Isrc
CFLAGS = -O3 -Wall -Wextra $(INCLUDES) $(SDLINC) $(SRCDIR)
LIBDIR = -L$(OO_PS4_TOOLCHAIN)/lib
LIBS = -lSDL2 -lm -lpthread
LDFLAGS = $(LIBDIR) $(LIBS)

all: WISH30300.pkg

assets_gen:
	python3 tools/generate_assets.py

eboot.bin: src/main.c assets_gen
	$(CC) $(CFLAGS) src/main.c -o eboot.bin $(LDFLAGS)

PARAM.SFO:
	python3 tools/gen_sfo.py

project.gp4: eboot.bin PARAM.SFO
	python3 tools/make_gp4.py

WISH30300.pkg: project.gp4
	python3 tools/pkg_builder.py

clean:
	rm -f eboot.bin PARAM.SFO project.gp4 WISH30300.pkg
	rm -rf pkg_root sc0 assets
