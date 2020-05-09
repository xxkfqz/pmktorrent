# pmktorrent
A simple command line utility to create BitTorrent metainfo files . It is a maintained fork of [mktorrent](https://github.com/Rudde/mktorrent)

## Features
- Creates a BitTorrent metainfo file from a file (block devices are also
 available) or directory in a simple
 and fast way
- Supports multiple trackers
- Can add a custom comment/creation date/created by to the metainfo file
- Can add the private flag to dissalow DHT and Peer Exchange
- Can add web seed URLs
- Hashing can be done multi threaded and supports multiple CPUs

## How to build
```shell script
# on most systems
make
make install

# on 32bit systems
make USE_LARGE_FILES=1
make install
```

## Build options
### Enabled by default
- `USE_PTHREADS` - Use multiple POSIX threads for calculating hashes. This should be slightly faster. Much faster on systems with multiple CPUs and fast harddrives
- `USE_LONG_OPTIONS` - Enable long options, started with two dashes
- `PROGRESS_PERIOD` - Set the number of microseconds pmktorrent will wait between every progress report when hashing multithreaded. Default is 200000, that is 0.2 seconds between every update
- `MAX_OPENFD` - Maximum number of file descriptors pmktorrent will open when scanning the directory. Default is 100, but I have no idea what a sane default for this value is, so your number is probably better

### Disabled by default
- `USE_OPENSSL` - *see below*
- `USE_LARGE_FILES` - This is needed on certain 32bit OSes (notably 32bit Linux) to support files and torrents > 2Gb
- `NO_HASH_CHECK` - Disable a redundant check to see if the amount of bytes read from files while hashing matches the sum of reported file sizes. I've never seen this fail. It will fail if you change files yet to be hashed while pmktorrent is running, but who'd want to do that?
- `DEBUG` - Enable leftover debugging code. Usually just spams you with lots of useless information

If you use an old version of BSD's make, you might need:
`make -f BSDmakefile`

## OpenSSL

pmktorrent can optionally use the SHA1 algorithm in the OpenSSL library instead of compiling its own. Most systems have this library installed already, but on some systems (notably Ubuntu and other Debian derivatives) you’ll also need the development package in order to build the program. Usually it will be called something like `openssl-dev` or `libssl-dev`. For this you need to specify the `USE_OPENSSL=1` option