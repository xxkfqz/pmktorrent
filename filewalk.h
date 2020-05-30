#ifndef _FILEWALK_H
#define _FILEWALK_H

typedef int (*file_tree_walk_cb)(const char *name,
                                 const struct stat *sbuf, void *data);

#ifndef ALLINONE
int file_tree_walk(const char *dirname, unsigned int nfds,
                   file_tree_walk_cb callback, void *data);

#endif /* ALLINONE */

#endif /* _FILEWALK_H */
