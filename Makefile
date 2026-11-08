CC = clang
CFLAGS = -O3 -Wall -Wextra --target=x86_64-pc-freebsd12-elf -Isrc -I$(OO_PS4_TOOLCHAIN)/include
LDFLAGS = -L$(OO_PS4_TOOLCHAIN)/lib -lm -lpthread -lSceAudioOut -lSceUserService

all: WISH30300.pkg

eboot.bin: src/main.c
	$(CC) $(CFLAGS) src/main.c -o eboot.bin $(LDFLAGS)

PARAM.SFO: tools/gen_sfo.py
	python3 tools/gen_sfo.py

project.gp4: tools/make_gp4.py eboot.bin PARAM.SFO
	python3 tools/make_gp4.py

WISH30300.pkg: project.gp4
	echo "Mock PKG creation for OpenOrbis workflow"
	touch WISH30300.pkg

clean:
	rm -f eboot.bin PARAM.SFO project.gp4 WISH30300.pkg
	rm -rf pkg_root sc0
