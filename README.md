# pmktorrent
A simple command line utility to create BitTorrent metainfo files. It is a fork of [mktorrent](https://github.com/Rudde/mktorrent).

# How to build
```
make
make install
```

## Build options
### Enabled by default
- `USE_PTHREADS` - Use multiple POSIX threads for calculating hashes. This should be slightly faster. Much faster on systems with multiple CPUs and fast harddrives.
- `USE_LONG_OPTIONS` - Enable long options, started with two dashes.
- `PROGRESS_PERIOD` - Set the number of microseconds pmktorrent will wait between every progress report when hashing multithreaded. Default is 200000, that is 0.2 seconds between every update.
- `MAX_OPENFD` - Maximum number of file descriptors pmktorrent will open when scanning the directory. Default is 100, but I have no idea what a sane default for this value is, so your number is probably better.

### Disabled by default
- `USE_OPENSSL` - Use the SHA1 implementation in the OpenSSL library instead of compiling our own.
- `USE_LARGE_FILES` - This is needed on certain 32bit OSes (notably 32bit Linux) to support files and torrents > 2Gb.
- `NO_HASH_CHECK` - Disable a redundant check to see if the amount of bytes read from files while hashing matches the sum of reported file sizes. I've never seen this fail. It will fail if you change files yet to be hashed while pmktorrent is running, but who'd want to do that?
- `DEBUG` - Enable leftover debugging code. Usually just spams you with lots of useless information.

If you use an old version of BSD's make, you might need:

`make -f BSDmakefile`
