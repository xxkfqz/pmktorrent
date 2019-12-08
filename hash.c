/*
This file is part of pmktorrent
Copyright (C) 2007, 2009 Emil Renner Berthing
Edited 2019 xxkfqz <xxkfqz@gmail.com>

pmktorrent is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pmktorrent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef ALLINONE
#include <stdlib.h>       /* exit() */
#include <sys/types.h>    /* off_t */
#include <errno.h>        /* errno */
#include <string.h>       /* strerror() */
#include <stdio.h>        /* printf() etc. */
#include <fcntl.h>        /* open() */
#include <unistd.h>       /* read(), close() */
#include <inttypes.h>     /* PRId64 etc. */

#ifdef USE_OPENSSL
#include <openssl/sha.h>  /* SHA1() */
#else
#include "sha1.h"
#endif

#include "pmktorrent.h"

#define EXPORT
#endif /* ALLINONE */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined _LARGEFILE_SOURCE && defined O_LARGEFILE
#define OPENFLAGS (O_RDONLY | O_BINARY | O_LARGEFILE)
#else
#define OPENFLAGS (O_RDONLY | O_BINARY)
#endif

/*
 * go through the files in file_list, split their contents into pieces
 * of size piece_length and create the hash string, which is the
 * concatenation of the (20 byte) SHA1 hash of every piece
 * last piece may be shorter
 */
EXPORT unsigned char *make_hash(metafile_t *m)
{
        flist_t *f;                     /* pointer to a place in the file list */
        unsigned char *hash_string;     /* the hash string */
        unsigned char *pos;             /* position in the hash string */
        unsigned char *read_buf;        /* read buffer */
        int fd;                         /* file descriptor */
        size_t r;                       /* number of bytes read from file(s) into
                                           the read buffer */
        SHA_CTX c;                      /* SHA1 hashing context */
#ifndef NO_HASH_CHECK
        int64_t counter = 0;            /* number of bytes hashed
                                           should match size when done */
#endif

        /* allocate memory for the hash string
           every SHA1 hash is SHA_DIGEST_LENGTH (20) bytes long */
        hash_string = malloc(m->pieces * SHA_DIGEST_LENGTH);
        /* allocate memory for the read buffer to store 1 piece */
        read_buf = malloc(m->piece_length);

        /* check if we've run out of memory */
        if (hash_string == NULL || read_buf == NULL) {
                fprintf(stderr, "Out of memory.\n");
                exit(EXIT_FAILURE);
        }

        /* initiate pos to point to the beginning of hash_string */
        pos = hash_string;
        /* and initiate r to 0 since we haven't read anything yet */
        r = 0;
        /* go through all the files in the file list */
        for (f = m->file_list; f; f = f->next) {

                /* open the current file for reading */
                if ((fd = open(f->path, OPENFLAGS)) == -1) {
                        fprintf(stderr, "Error opening '%s' for reading: %s\n",
                                        f->path, strerror(errno));
                        exit(EXIT_FAILURE);
                }
                fprintf(stderr, "Hashing %s.\n", f->path);

                /* fill the read buffer with the contents of the file and append
                   the SHA1 hash of it to the hash string when the buffer is full.
                   repeat until we can't fill the read buffer and we've thus come
                   to the end of the file */
                while (1) {
                        ssize_t d = read(fd, read_buf + r, m->piece_length - r);

                        if (d < 0) {
                                fprintf(stderr, "Error reading from '%s': %s\n",
                                                f->path, strerror(errno));
                                exit(EXIT_FAILURE);
                        }

                        if (d == 0) /* end of file */
                                break;

                        r += d;

                        if (r == m->piece_length) {
                                SHA1_Init(&c);
                                SHA1_Update(&c, read_buf, m->piece_length);
                                SHA1_Final(pos, &c);
                                pos += SHA_DIGEST_LENGTH;
#ifndef NO_HASH_CHECK
                                counter += r;	/* r == piece_length */
#endif
                                r = 0;
                        }
                }

                /* now close the file */
                if (close(fd)) {
                        fprintf(stderr, "Error closing '%s': %s\n",
                                        f->path, strerror(errno));
                        exit(EXIT_FAILURE);
                }
        }

        /* finally append the hash of the last irregular piece to the hash string */
        if (r) {
                SHA1_Init(&c);
                SHA1_Update(&c, read_buf, r);
                SHA1_Final(pos, &c);
        }

#ifndef NO_HASH_CHECK
        counter += r;
        if (counter != m->size) {
                fprintf(stderr, "Counted %" PRId64 " bytes, "
                                "but hashed %" PRId64 " bytes. "
                                "Something is wrong...\n", m->size, counter);
                exit(EXIT_FAILURE);
        }
#endif

        /* free the read buffer before we return */
        free(read_buf);

        return hash_string;
}
