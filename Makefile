pkg_cflags  ?= $(shell pkg-config --cflags $(1))
pkg_ldflags ?= $(shell pkg-config --libs $(1))

CC ?= gcc

PATH_SRC     = src
PATH_EXTRA   = extras/statusnotifier/src
PATH_BUILD  ?= build
PATH_BUILD2 ?= $(PATH_BUILD)/gtk2
PATH_BUILD3 ?= $(PATH_BUILD)/gtk3

OUT_GTK2 ?= sni_gtk2.so
OUT_GTK3 ?= sni_gtk3.so

APP_GLIB_MKENUMS ?= /usr/bin/glib-mkenums

LIST_CLEAN = CMakeCache.txt         \
			 CMakeFiles             \
			 cmake_install.cmake    \
			 cmake_uninstall.cmake  \
			 install_manifest.txt   \
			 build                  \
			 _build                 \
			 *.so                   \
			 *.swp                  \
			 enums.[c,h]            \
			 .ninja_deps            \
			 .ninja_log             \
			 *.ninja

SNI_DEFS += -DUSE_DBUSMENU -DENABLE_NLS -DG_LOG_DOMAIN=\"plugin-sni\"
SNI_DEPS += dbusmenu-glib-0.4 x11 

CFLAGS   ?= -Wall -Wextra -g -O2 -fPIC -std=c99 -D_GNU_SOURCE -Wno-unused -fvisibility=hidden
CFLAGS   +=$(call pkg_cflags, $(SNI_DEPS))
LDFLAGS  ?= -shared -s -fdata-sections -ffunction-sections -Wl,-gc-sections
INCLUDES += -I $(abspath ./) -I $(abspath $(PATH_SRC)) -I $(abspath $(PATH_EXTRA))

GTK2 = gtk+-2.0
GTK3 = gtk+-3.0

SNI_SRC_LIST = menu.c sni.c
SNI_EXT_LIST = statusnotifier.c closures.c
SNI_SRC = enums.c $(addprefix $(PATH_SRC)/,$(SNI_SRC_LIST)) $(addprefix $(PATH_EXTRA)/,$(SNI_EXT_LIST))
OBJ_GTK2 = $(patsubst %.c, $(PATH_BUILD2)/%.o, $(SNI_SRC))
OBJ_GTK3 = $(patsubst %.c, $(PATH_BUILD3)/%.o, $(SNI_SRC))

### FUNCTIONS
define compile
    $(CC) $(CFLAGS) $(SNI_DEFS) $(INCLUDES) $1 $(abspath $2) -c -o $(abspath $@) 
endef
define link
    $(CC) $(LDFLAGS) $(abspath $^) $1 -o $(abspath $@) 
endef

### TARGETS

all: gtk3 gtk2

mkenums: enums.c enums.h 
enums.h: $(PATH_EXTRA)/enums.h.template $(PATH_EXTRA)/statusnotifier.h
	@$(APP_GLIB_MKENUMS) --template $^ > $@
enums.c: $(PATH_EXTRA)/enums.c.template $(PATH_EXTRA)/statusnotifier.h
	@$(APP_GLIB_MKENUMS) --template $^ > $@

.PHONY: clean mkdirs_gtk3 mkdirs_gtk3
clean:
	@rm -rf $(LIST_CLEAN)

gtk3: mkdirs_gtk3 mkenums $(PATH_BUILD)/$(OUT_GTK3)
gtk2: mkdirs_gtk2 mkenums $(PATH_BUILD)/$(OUT_GTK2)

mkdirs_gtk3:
	@mkdir -p $(abspath $(PATH_BUILD3)/$(PATH_SRC))
	@mkdir -p $(abspath $(PATH_BUILD3)/$(PATH_EXTRA))
mkdirs_gtk2:
	@mkdir -p $(abspath $(PATH_BUILD2)/$(PATH_SRC))
	@mkdir -p $(abspath $(PATH_BUILD2)/$(PATH_EXTRA))

## BUILD TARGETS

$(PATH_BUILD)/$(OUT_GTK3): $(OBJ_GTK3)
	@$(call link, $(call pkg_ldflags, $(GTK3)) $(call pkg_ldflags, $(SNI_DEPS)))

$(PATH_BUILD3)/%.o: $(SNI_SRC)
	@$(call compile, $(call pkg_cflags, $(GTK3)), $(filter %$(@F:%.o=%.c), $^))

$(PATH_BUILD)/$(OUT_GTK2): $(OBJ_GTK2)
	@$(call link, $(call pkg_ldflags, $(GTK2)) $(call pkg_ldflags, $(SNI_DEPS)))

$(PATH_BUILD2)/%.o: $(SNI_SRC)
	@$(call compile, $(call pkg_cflags, $(GTK2)), $(filter %$(@F:%.o=%.c), $^))
