/*
This file is part of pmktorrent
Copyright (C) 2007, 2009 Emil Renner Berthing
Edited 2019-2020 xxkfqz <xxkfqz@gmail.com>

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
#include <stdio.h>        /* perror(), printf() etc. */
#include <sys/stat.h>     /* the stat structure */
#include <sys/mount.h>    /* the BLKGETSIZE64 constant and ioctl() */
#include <fcntl.h>        /* the O_RDONLY flag */
#include <unistd.h>       /* getopt(), getcwd(), sysconf() */
#include <string.h>       /* strcmp(), strlen(), strncpy() */
#include <strings.h>      /* strcasecmp() */
#include <ctype.h>        /* isspace() */
#include <stdbool.h>
#include <inttypes.h>     /* PRId64 etc. */
#include <time.h>         /* time(NULL) */
#include <limits.h>       /* PATH_MAX */
#include <math.h>         /* fmax(), fmin(), log() etc. */
#include <ftw.h>          /* ftw() */
#ifdef USE_LONG_OPTIONS
#include <getopt.h>       /* getopt_long() */
#endif

#include "pmktorrent.h"
#include "filewalk.h"

#define EXPORT
#endif /* ALLINONE */

#define MAX_ANNOUNCE_FILE_SIZE (1024 * 16)
#ifndef MAX_OPENFD
#define MAX_OPENFD 100	/* Maximum number of file descriptors
                           file_tree_walk() will open */
#endif

/* human readable string maximum size */
#define HR_STR_SIZE 12

unsigned long long ftw_total_size = 0;

static void strip_ending_dirseps(char *s)
{
    char *end = s;

    while (*end)
        end++;

    while (end > s && *(--end) == DIRSEP_CHAR)
        *end = '\0';
}

static void trim_right(char* s, char* end)
{
    while (end != s && (isspace((unsigned char)*end) || *end == '\0'))
    {
        *end = '\0';
        --end;
    }

    if (isspace((unsigned char)*end))
    {
        *end = '\0';
    }
}

static int get_filesize(FILE* f, long* filesize)
{
    long pos = ftell(f);
    if (pos == EOF)
    {
        return -1;
    }
    int err = fseek(f, 0, SEEK_END);
    if (err != 0)
    {
        return -1;
    }
    *filesize = ftell(f);
    if (*filesize == EOF)
    {
        return -1;
    }
    return fseek(f, 0, SEEK_SET);
}

static const char *basename(const char *s)
{
    const char *r = s;

    while (*s != '\0')
    {
        if (*s == DIRSEP_CHAR)
            r = ++s;
        else
            ++s;
    }

    return r;
}

static void set_absolute_file_path(metafile_t *m)
{
    /* string to return */
    char *string;

    /* length of the string */
    size_t length = 32;

    /* if the file_path is already an absolute path just
       return that */
    if (m->metainfo_file_path && *m->metainfo_file_path == DIRSEP_CHAR)
        return;

    /* return early if path is "-" (meaning stdout), either */
    if (m->metainfo_file_path
            && strcmp ("-", m->metainfo_file_path) == 0)
    {
        return;
    }

    /* first get the current working directory
       using getcwd is a bit of a PITA */
    /* allocate initial string */
    string = malloc(length);
    if (string == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }
    /* while our allocated memory for the working dir isn't big enough */
    while (getcwd(string, length) == NULL)
    {
        /* double the buffer size */
        length *= 2;
        /* free our current buffer */
        free(string);
        /* and allocate a new one twice as big muahaha */
        string = malloc(length);
        if (string == NULL)
        {
            fprintf(stderr, "Out of memory.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* now set length to the proper length of the working dir */
    length = strlen(string);
    /* if the metainfo file path isn't set */
    if (m->metainfo_file_path == NULL)
    {
        /* append <torrent name>.torrent to the working dir */
        string = realloc(string, length + strlen(m->torrent_name) + 10);
        if (string == NULL)
        {
            fprintf(stderr, "Out of memory.\n");
            exit(EXIT_FAILURE);
        }
        sprintf(string + length, DIRSEP "%s.torrent", m->torrent_name);
    }
    else
    {
        /* otherwise append the torrent path to the working dir */
        string = realloc(string, length + strlen(m->metainfo_file_path) + 2);
        if (string == NULL)
        {
            fprintf(stderr, "Out of memory.\n");
            exit(EXIT_FAILURE);
        }
        sprintf(string + length, DIRSEP "%s", m->metainfo_file_path);
    }
    m->metainfo_file_path = string;
}

/*
 * parse a separator separated list of strings and
 * return a string list containing the substrings
 * whitspace characters get removed when trim is set to true
 */
static slist_t *get_slist(char *s, char separator, bool trim)
{
    slist_t *list, *last;
    char *e;

    /* allocate memory for the first node in the list */
    list = last = malloc(sizeof(slist_t));
    if (list == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }

    /*
     * add URLs to the list while there are separator characters in the
     * string
     */
    while ((e = strchr(s, separator)))
    {
        /* set the separator to \0 so the URLs appear as
         * separate strings */
        *e = '\0';
        if (trim)
        {
            trim_right(s, e);
        }
        last->s = s;

        /* move s to point to the next URL */
        s = e + 1;

        /* append another node to the list */
        last->next = malloc(sizeof(slist_t));
        last = last->next;
        if (last == NULL)
        {
            fprintf(stderr, "Out of memory.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* set the last string in the list */
    last->s = s;
    if (trim)
    {
        trim_right(last->s, last->s + strlen(last->s));
    }
    last->next = NULL;

    /* return the list */
    return list;
}

/*
 * checks if target is a directory
 * sets the file_list and size if it isn't
 */
static int is_dir(metafile_t *m, char *target)
{
    struct stat s;       /* stat structure for stat() to fill */

    /* stat the target */
    if (stat(target, &s))
    {
        fprintf(stderr, "Error stat'ing '%s': %s\n",
                target, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* if it is a directory, just return 1 */
    if (S_ISDIR(s.st_mode))
        return 1;

    /* if it isn't a regular file either, something is wrong.. */
    if (!S_ISREG(s.st_mode) && !S_ISBLK(s.st_mode))
    {
        fprintf(stderr,
                "'%s' is neither a directory nor regular file nor a block device.\n",
                target);
        exit(EXIT_FAILURE);
    }

    /* since we know the torrent is just a single file and we've
       already stat'ed it, we might as well set the file list */
    m->file_list = malloc(sizeof(flist_t));
    if (m->file_list == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }
    m->file_list->path = target;
    m->file_list->next = NULL;
    /* ..and size variable */
    if (S_ISREG(s.st_mode))
    {
        m->size = s.st_size;
    }
    else
    {
        int fd;
        if (0 > (fd = open(target, O_RDONLY)) ||
                0 > ioctl(fd, BLKGETSIZE64, &(m->size)) ||
                0 > close(fd))
        {
            fprintf(stderr, "could not read block device size '%s': %s\n",
                    target, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    m->file_list->size = m->size;
    m->file_count = 1;

    /* now return 0 since it isn't a directory */
    return 0;
}

/*
 * called by file_tree_walk() on every file and directory in the subtree
 * counts the number of (readable) files, their commulative size and adds
 * their names and individual sizes to the file list
 */
static int process_node(const char *path, const struct stat *sb, void *data)
{
    flist_t **p;            /* pointer to a node in the file list */
    flist_t *new_node;      /* place to store a newly created node */
    metafile_t *m = data;

    /* skip non-regular files */
    if (!S_ISREG(sb->st_mode))
        return 0;

    /* ignore the leading "./" */
    path += 2;

    /* now path should be readable otherwise
     * display a warning and skip it */
    if (access(path, R_OK))
    {
        if (!m->quiet)
            fprintf(stderr,
                    "Warning: Cannot read '%s', skipping.\n", path);
        return 0;
    }

    if (m->verbose)
        fprintf(stderr, "Adding %s\n", path);

    /* count the total size of the files */
    m->size += sb->st_size;

    /* just insert the new node at the head of the list and sort it later */
    p = &m->file_list;

    /* create a new file list node for the file */
    /* TODO: fix memory leaks on 'malloc' and 'strdup' */
    new_node = malloc(sizeof(flist_t));
    if (new_node == NULL || (new_node->path = strdup(path)) == NULL)
    {
        fprintf(stderr, "Out of memory.\n");
        free(new_node->path);
        return -1;
    }
    new_node->size = sb->st_size;

    /* now insert the node there */
    new_node->next = *p;
    *p = new_node;
    m->file_count += 1;

    return 0;
}

static int cmp_flist(const void *left, const void *right)
{
    flist_t *f_left = *(flist_t **)left;
    flist_t *f_right = *(flist_t **)right;
    return strcmp(f_left->path, f_right->path);
}

static int sort_file_list(void *data)
{
    flist_t **nodes;
    flist_t *current_node;
    int i = 0;
    metafile_t *m = data;

    /* no need to sort a list with a single item */
    if (m->file_count <= 1)
        return 0;

    nodes = malloc(sizeof(flist_t*) * m->file_count);
    if (nodes == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    /* stuff the linked list into an array for easier sorting */
    current_node = m->file_list;
    while (current_node)
    {
        nodes[i++] = current_node;
        current_node = current_node->next;
    }

    /* sort the node pointers in the array */
    // if (mergesort(nodes, m->file_count, sizeof(flist_t*), cmp_flist))
    // 	exit(EXIT_FAILURE);
    qsort(nodes, m->file_count, sizeof(flist_t*), cmp_flist);

    /* fix the .next pointers on each node */
    for (i = 1; i < (int)m->file_count; i++)
    {
        nodes[i-1]->next = nodes[i];
    }
    nodes[i-1]->next = NULL;
    m->file_list = nodes[0];

    /* release the array since we don't need it anymore */
    free(nodes);

    return 0;
}

/*
 * 'elp!
 */
static void print_help()
{
    printf(
        "Usage: " PROGRAM " [OPTIONS] <target directory, block device or filename>\n\n"
        "Options:\n"
#ifdef USE_LONG_OPTIONS
        "  -a, --announce=<url>[,<url>]* - specify the full announce URLs\n"
        "                                  additional -a adds backup trackers\n"
        "  -A, --announce-file=<file>    - specify a file from which a full announce URL\n"
        "                                  is read\n"
        "  -c, --comment=<comment>       - add a comment to the metainfo\n"
        "  -d, --date=[<timestamp>]      - specify the creation date\n"
        "  -D, --no-date                 - don't write the creation date\n"
        "  -f, --force                   - overwrite the output file if it exists\n"
        "  -h, --help                    - show this help screen\n"
        "  -l, --piece-length=<n>|auto   - set the piece length to 2^n bytes,\n"
        "                                  default is 'auto'\n"
        "  -n, --name=<name>             - set the name of the torrent\n"
        "                                  default is the basename of the target\n"
        "  -o, --output=<filename>       - set the path and filename of the created file\n"
        "                                  default is <name>.torrent\n"
        "  -p, --private                 - set the private flag\n"
        "  -s, --source=<source>         - add source string embedded in infohash\n"
        "  -b, --created-by=<name>       - set the name of the program that created the\n"
        "                                  file aka \"Created by\" field (optional)\n"
        "                                  default: \"" PROGRAM" "VERSION"\"\n"
#ifdef USE_PTHREADS
        "  -t, --threads=<n>             - use <n> threads for calculating hashes\n"
        "                                  default is the number of CPU cores\n"
#endif /* USE_PTHREADS */
        "  -V, --version                 - print the version number and exit\n"
        "  -v, --verbose                 - be verbose\n"
        "  -q, --quiet                   - be quiet (do not print anything)\n"
        "  -w, --web-seed=<url>[,<url>]* - add web seed URLs\n"
        "                                  additional -w adds more URLs\n"
#else /* USE_LONG_OPTIONS */
        "  -a <url>[,<url>]* - specify the full announce URLs\n"
        "                      additional -a adds backup trackers\n"
        "  -A <file>         - specify a file from which a full announce URL is read\n"
        "  -c <comment>      - add a comment to the metainfo\n"
        "  -f                - overwrite the output file if it exists\n"
        "  -d [<timestamp>]  - overwrite the creation date\n"
        "  -D                - don't write the creation date\n"
        "  -h                - show this help screen\n"
        "  -l <n>|auto       - set the piece length to 2^n bytes,\n"
        "                      default is 'auto'\n"
        "  -n <name>         - set the name of the torrent,\n"
        "                      default is the basename of the target\n"
        "  -o <filename>     - set the path and filename of the created file\n"
        "                      default is <name>.torrent\n"
        "  -p                - set the private flag\n"
        "  -s                - add source string embedded in infohash\n"
        "  -b <name>         - set the name of the program that created the file aka\n"
        "                      \"Created by\" field (optional)\n"
        "                      default: \""PROGRAM " " VERSION"\"\n"
#ifdef USE_PTHREADS
        "  -t <n>            - use <n> threads for calculating hashes\n"
        "                      default is the number of CPU cores\n"
#endif /* USE_PTHREADS */
        "  -V                - print the version number and exit"
        "  -v                - be verbose\n"
        "  -q                - be quiet (do not print anything)\n"
        "  -w <url>[,<url>]* - add web seed URLs\n"
        "                      additional -w adds more URLs\n"
#endif /* USE_LONG_OPTIONS */
        "\nCopyright (C) 2007, 2009 Emil Renner Berthing\n"
        "Edited 2019-2020 xxkfqz <xxkfqz@gmail.com>\n\n"
        "Please send bug reports, patches, feature requests, praise and\n"
        "general gossip about the program to: "
        "https://github.com/xxkfqz/pmktorrent\n");
}

/*
 * print the full announce list
 */
static void print_announce_list(llist_t *list)
{
    unsigned int n;

    fputs("  Announce URLs:\n", stderr);
    for (n = 1; list; list = list->next, n++)
    {
        slist_t *l = list->l;

        fprintf(stderr, "    %u : %s\n", n, l->s);
        for (l = l->next; l; l = l->next)
            fprintf(stderr, "        %s\n", l->s);
    }
}

/*
 * print the list of web seed URLs
 */
static void print_web_seed_list(slist_t *list)
{
    fprintf(stderr, "  Web Seed URL:  ");

    if (list == NULL)
    {
        fprintf(stderr, "none\n");
        return;
    }

    fprintf(stderr, "%s\n", list->s);
    for (list = list->next; list; list = list->next)
        fprintf(stderr, "                %s\n", list->s);
}

/*
 * print out all the options
 */
static void dump_options(metafile_t *m)
{
    fputs("Options:\n", stderr);

    if (m->announce_list != NULL)
        print_announce_list(m->announce_list);

    fprintf(stderr,
            "  Torrent name:  %s\n"
            "  Metafile:      %s\n"
            "  Piece length:  %u\n"

#ifdef USE_PTHREADS
            "  Threads:       %u\n"
#endif

            "  Be verbose:    yes\n",
            m->torrent_name, m->metainfo_file_path, m->piece_length

#ifdef USE_PTHREADS
            ,m->threads
#endif
           );

    /* Creation date block */
    time_t t = m->creation_date;
    struct tm *tm = localtime(&t);
    char date_string[29];
    strftime(date_string, sizeof(date_string), "%Y.%m.%d %H:%M:%S", tm);

    fprintf(stderr, "  Creation date: ");
    if(m->no_creation_date)
        fprintf(stderr, "none\n");
    else
        fprintf(stderr, "%s\n  Timestamp:     %ld\n", date_string,
                m->creation_date);
    /* end */

    print_web_seed_list(m->web_seed_list);

    /* Print source string only if set */
    if (m->source)
        fprintf(stderr, "\n Source:       %s\n\n", m->source);

    fprintf(stderr, "  Comment:       ");
    if (m->comment == NULL)
        fprintf(stderr, "none\n");
    else
        fprintf(stderr, "\"%s\"\n", m->comment);

    fprintf(stderr, "  Created by:    \"%s\"\n", m->created_by);
}

/*
 * adopted for C variants:
 * https://programming.guide/java/formatting-byte-size-to-human-readable-format.html
 */
static char *getSiSize(uint64_t b)
{
    char *s = calloc(HR_STR_SIZE, sizeof(char));

    b < 1000 ? snprintf(s, HR_STR_SIZE, "%" PRIu64 " B", b)
    : b < 999950 ? snprintf(s, HR_STR_SIZE, "%.1lf KB", (double)(b) / 1000)
    : (b /= 1000) < 999950 ? snprintf(s, HR_STR_SIZE, "%.1lf MB", (double)(b) / 1000)
    : (b /= 1000) < 999950 ? snprintf(s, HR_STR_SIZE, "%.1lf GB", (double)(b) / 1000)
    : (b /= 1000) < 999950 ? snprintf(s, HR_STR_SIZE, "%.1lf TB", (double)(b) / 1000)
    : (b /= 1000) < 999950 ? snprintf(s, HR_STR_SIZE, "%.1lf PB", (double)(b) / 1000)
    : snprintf(s, HR_STR_SIZE, "%.1lf EB", (double)(b / 1e6));

    return s;
}

static char *getBinarySize(uint64_t b)
{
    char *s = calloc(HR_STR_SIZE, sizeof(char));

    b < 1024 ? snprintf(s, HR_STR_SIZE, "%" PRIu64 " B", b)
    : b <= 0xfffcccccccccccc >> 40 ? snprintf(s, HR_STR_SIZE, "%.1lf KiB", (double)(b) / 0x1p10)
                             : b <= 0xfffcccccccccccc >> 30 ? snprintf(s, HR_STR_SIZE, "%.1lf MiB", (double)(b) / 0x1p20)
                             : b <= 0xfffcccccccccccc >> 20 ? snprintf(s, HR_STR_SIZE, "%.1lf GiB", (double)(b) / 0x1p30)
                             : b <= 0xfffcccccccccccc >> 10 ? snprintf(s, HR_STR_SIZE, "%.1lf TiB", (double)(b) / 0x1p40)
                             : b <= 0xfffcccccccccccc ? snprintf(s, HR_STR_SIZE, "%.1lf PiB", (b >> 10) / 0x1p40)
                             : snprintf(s, HR_STR_SIZE, "%.1lf EiB", (double)(b >> 20) / 0x1p40);

    return s;
}

static int calculate_size(const char *fpath, const struct stat *sb, int typeflag)
{
    ftw_total_size += sb->st_size;
    return 0;
}

static int calculate_piece_length(const char *path)
{
    if(ftw(path, &calculate_size, MAX_OPENFD))
    {
        fprintf(stderr, "Cannot calculate size '%s'\n", path);
        exit(EXIT_FAILURE);
    }

    /* https://github.com/Rudde/mktorrent/issues/30#issue-400978650 */
    return fmax( 15, fmin( 24, floor( log( ftw_total_size / 1000 ) / log( 2 ))));
}

/*
 * parse and check the command line options given and fill out the appropriate
 * fields of the metafile structure
 */
EXPORT void init(metafile_t *m, int argc, char *argv[])
{
	if(argc <= 1)
	{
		print_help();
		exit(EXIT_SUCCESS);
	}
	
    /* return value of getopt() */
    int c;

    llist_t *announce_last = NULL;
    slist_t *web_seed_last = NULL;
    char *optargptr = NULL;
#ifdef USE_LONG_OPTIONS
    /* the option structure to pass to getopt_long() */
    static struct option long_options[] =
    {
        {"announce", 1, NULL, 'a'},
        {"announce-file", 1, NULL, 'A'},
        {"comment", 1, NULL, 'c'},
        {"no-date", 0, NULL, 'D'},
        {"date", 1, NULL, 'd'},
        {"force", 0, NULL, 'f'},
        {"help", 0, NULL, 'h'},
        {"piece-length", 1, NULL, 'l'},
        {"name", 1, NULL, 'n'},
        {"output", 1, NULL, 'o'},
        {"private", 0, NULL, 'p'},
        {"source", 1, NULL, 's'},
        {"created-by", 2, NULL, 'b'},
#ifdef USE_PTHREADS
        {"threads", 1, NULL, 't'},
#endif
        {"version", 0, NULL, 'V'},
        {"verbose", 0, NULL, 'v'},
        {"quite", 0, NULL, 'q'},
        {"web-seed", 1, NULL, 'w'},
        {NULL, 0, NULL, 0}
    };
#endif /* USE_LONG_OPTIONS */

    /* now parse the command line options given */
#ifdef USE_PTHREADS
#define OPT_STRING "A:a:c:Dd:fhl:n:o:ps:b::t:Vvqw:"
#else
#define OPT_STRING "A:a:c:Dd:fhl:n:o:ps:b::Vvqw:"
#endif
#ifdef USE_LONG_OPTIONS
    while ((c = getopt_long(argc, argv, OPT_STRING,
                            long_options, NULL)) != -1)
    {
#else
    while ((c = getopt(argc, argv, OPT_STRING)) != -1)
    {
#endif
#undef OPT_STRING
        switch (c)
        {
        case 'A':
        {
            free(m->announce_from_file);
            FILE* announce_file = fopen(optarg, "r");
            if (announce_file == NULL)
            {
                fprintf(stderr, "Couldn't open file for reading.\n");
                exit(EXIT_FAILURE);
            }
            long filesize;
            int err = get_filesize(announce_file, &filesize);
            if (err != 0)
            {
                fprintf(stderr, "Couldn't determine the filesize successfully.\n");
                exit(EXIT_FAILURE);
            }
            if (filesize > MAX_ANNOUNCE_FILE_SIZE)
            {
                fprintf(stderr, "The file is bigger than the maximum allowed size which is %d MiB.\n", MAX_ANNOUNCE_FILE_SIZE / 1024);
                fprintf(stderr, "Probably you've chosen a wrong file for the announce URL.\n");
                exit(EXIT_FAILURE);
            }
            char* announce = malloc(filesize + 1);
            if (announce == NULL)
            {
                fprintf(stderr, "Couldn't allocate the buffer for the file.\n");
                exit(EXIT_FAILURE);
            }
            size_t bytes_read = fread(announce, 1, MAX_ANNOUNCE_FILE_SIZE, announce_file);
            if (ferror(announce_file) != 0)
            {
                fprintf(stderr, "An error occured reading the file.\n");
                exit(EXIT_FAILURE);
            }
            announce[bytes_read] = '\0';
            trim_right(announce, announce + bytes_read);

            fclose(announce_file);
            if (announce_last == NULL)
            {
                m->announce_list = announce_last = malloc(sizeof(llist_t));
                announce_last->l = get_slist(announce, '\n', 0);
            }
            else
            {
                fprintf(stderr, "The announce file options must occur uniquely and it requires no announce set with -a.\n");
                exit(EXIT_FAILURE);
            }
            m->announce_from_file = announce;
            break;
        }
        case 'a':
            if (announce_last == NULL)
            {
                m->announce_list = announce_last =
                                       malloc(sizeof(llist_t));
            }
            else
            {
                announce_last->next =
                    malloc(sizeof(llist_t));
                announce_last = announce_last->next;

            }
            if (announce_last == NULL)
            {
                fprintf(stderr, "Out of memory.\n");
                exit(EXIT_FAILURE);
            }
            announce_last->l = get_slist(optarg, ',', false);
            break;
        case 'c':
            m->comment = optarg;
            break;
        case 'f':
            m->force_output = 1;
            break;
        case 'D':
            m->no_creation_date = 1;
            break;
        case 'd':
            if(m->no_creation_date)
                break;

            optargptr = optarg;
            while(isdigit(*optargptr)) optargptr++;
            if(optargptr > optarg)
                m->creation_date = atol(optarg);
            else
            {
                fprintf(stderr,
                        "Invalid timestamp: %s\n",
                        optarg);
                exit(EXIT_FAILURE);
            }

            break;
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 'l':
            m->piece_length = strcmp(optarg, "auto") ? atoi(optarg) : 0;
            break;
        case 'n':
            m->torrent_name = optarg;
            break;
        case 'o':
            m->metainfo_file_path = optarg;
            break;
        case 'p':
            m->private = 1;
            break;
        case 's':
            m->source = optarg;
            break;
        case 'b':
            m->created_by = optarg;
            break;
#ifdef USE_PTHREADS
        case 't':
            m->threads = atoi(optarg);
            break;
#endif
        case 'V':
            printf("%s\n", PROGRAM " " VERSION);
            exit(EXIT_SUCCESS);
        case 'v':
            m->verbose = 1;
            break;
        case 'q':
            m->quiet = 1;
            break;
        case 'w':
            if (web_seed_last == NULL)
            {
                m->web_seed_list = web_seed_last =
                                       get_slist(optarg, ',', false);
            }
            else
            {
                web_seed_last->next =
                    get_slist(optarg, ',', false);
                web_seed_last = web_seed_last->next;
            }
            while (web_seed_last->next)
                web_seed_last = web_seed_last->next;
            break;
        case '?':
        default:
            print_help();
            exit(EXIT_SUCCESS);
            break;
        }
    }

    if (m->created_by == NULL)
        m->created_by = PROGRAM " " VERSION;

    /* set the correct piece length. default is 2^18 = 256kb. */
    if (m->piece_length == 0)
        m->piece_length = calculate_piece_length(argv[optind]);
    else if (m->piece_length < 15 || m->piece_length > 28)
    {
        fprintf(stderr,
                "The piece length must be a number between "
                "15 and 28.\n");
        exit(EXIT_FAILURE);
    }
    m->piece_length = 1 << m->piece_length;

    if (announce_last != NULL)
        announce_last->next = NULL;

    /* ..and a file or directory from which to create the torrent */
    if (optind >= argc)
    {
        fprintf(stderr, "Missing operand. Try '-h' or '--help', "
                "for more information\n");
        exit(EXIT_FAILURE);
    }

#ifdef USE_PTHREADS
    /* check the number of threads */
    if (m->threads)
    {
        if (m->threads > 20)
        {
            fprintf(stderr, "The number of threads is limited to "
                    "at most 20\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
#ifdef _SC_NPROCESSORS_ONLN
        long sysconf_result = sysconf(_SC_NPROCESSORS_ONLN);
        m->threads = (unsigned int)sysconf_result;
        if (sysconf_result == -1)
#endif
            /* some sane default */
            m->threads = 2;
    }
#endif

    /* strip ending DIRSEP's from target */
    strip_ending_dirseps(argv[optind]);

    /* if the torrent name isn't set use the basename of the target */
    if (m->torrent_name == NULL)
        m->torrent_name = basename(argv[optind]);

    /* make sure m->metainfo_file_path is the absolute path to the file */
    set_absolute_file_path(m);

    /* if we should not be quiet print out all the options as we have set
     * them */
    if (!m->quiet)
        dump_options(m);

    /* check if target is a directory or just a single file */
    m->target_is_directory = is_dir(m, argv[optind]);
    if (m->target_is_directory)
    {
        /* change to the specified directory */
        if (chdir(argv[optind]))
        {
            fprintf(stderr, "Error changing directory to '%s': %s\n",
                    argv[optind], strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (file_tree_walk("." DIRSEP, MAX_OPENFD, process_node, m))
            exit(EXIT_FAILURE);

        if (sort_file_list(m))
            exit(EXIT_FAILURE);
    }

    /* calculate the number of pieces
       pieces = ceil( size / piece_length ) */
    m->pieces = (m->size + m->piece_length - 1) / m->piece_length;

    /* now print the size and piece count if we should not be quiet */
    if (m->verbose)
    {
        char *SI_size = getSiSize(m->size);
        char *binary_size = getBinarySize(m->size);

        fprintf(stderr, "\n%" PRIoff " bytes (%s / %s) in all.\n"
                "That's %u pieces of %u bytes each.\n\n",
                m->size, SI_size, binary_size, m->pieces,
                m->piece_length);

        free(SI_size);
        free(binary_size);
    }
}
