# This file is part of pmktorrent
# Copyright (C) 2007, 2009 Emil Renner Berthing
# Edited 2019-2020 xxkfqz <xxkfqz@gmail.com>
#
# pmktorrent is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# pmktorrent is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

include Makefile

CC      ?= cc
CFLAGS  ?= -O3 -march=native -Wall
LDFLAGS ?= -lm
INSTALL ?= install
PREFIX  ?= /usr/local

ifdef USE_PTHREADS
DEFINES += -DUSE_PTHREADS
SRCS := $(SRCS:hash.c=hash_pthreads.c)
LIBS += -lpthread
endif

ifdef USE_OPENSSL
DEFINES += -DUSE_OPENSSL
SRCS := $(SRCS:sha1.c=)
LIBS += -lcrypto
endif

ifdef USE_LONG_OPTIONS
DEFINES += -DUSE_LONG_OPTIONS
endif

ifdef USE_LARGE_FILES
DEFINES += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
endif

ifdef NO_HASH_CHECK
DEFINES += -DNO_HASH_CHECK
endif

ifdef MAX_OPENFD
DEFINES += -DMAX_OPENFD="$(MAX_OPENFD)"
endif

ifdef DEBUG
DEFINES += -DDEBUG
endif

OFFPRFX = $(shell ./prefix)

OBJS = $(SRCS:.c=.o)

all: $(program)

%.o: %.c $(HEADERS) prefix
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFINES) -DPRIoff="\"$(OFFPRFX)d\"" -DPROGRAM="\"$(program)\"" -DVERSION="\"$(version)\"" -c $<

include rules.mk
