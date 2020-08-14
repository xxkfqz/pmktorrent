# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [1.3.1] - 2020-08-14
### Added
- Option to change creation date (`-d`)
- Added `auto` in piece length option
### Changed
- "Disable creation date" option changed from `-d` to `-D`
### Fixed
- Metafile path when output file specified like `./output.torrent` (issue
  [#1](https://github.com/xxkfqz/pmktorrent/issues/1))
- '(null)' in 'Metafile:'
- SIGSEGV when no arguments specified

## [1.3] - 2019-12-25
### Added
- Option for disabling any information output (`-q`)
- Option for overwrite the output file if it exists (`-f`)
- Option for print the version number (`-V`)
- Human readable total size of processed files in output file information
### Changed
- Symbolic links processing removed
### Fixed
- 'Too many levels of symbolic links' bug

## [1.2] - 2019-12-08
### Added
- Support for block devices (#9 by @infinity0)
- Implement quicksort for faster file path sorting (#33 by @aheadley)
- Optional "Created by" field (`-b`)
- `CHANGELOG.md`
- Warning message when files number can causes problems for users of other
  clients (issue #34 by @colindean)
### Changed
- Project name
- Help page
- "Missing operator" message
- Verbose message
- README
### Fixed
- Announce help removed from short options (#26 by @p-roesink)
- Writing to stdout instead of file (`--output=-`) (issue #36 by Ivan Shmakov
  <ivan@siamics.net>)

## [1.1] - 2017-01-11
### Added
- Autodetect the number of CPUs available <esmil@mailme.dk>
- Option for source string added to torrent info (`-s`), included in infohash.
  Often used by private trackers to create a unique infohash to prevent
  peer-leak and the possibility to track the trackers that do use leaked
  torrents. Having this option in mktorrent make it possible to create a
  infohash accurate torrent to the tracker you want to upload it to
### Changed
- Make`-a` (announce list) optional
- Optional announce URL even for private torrents. No need to require announce
  for private torrents, they are added by most private trackers anyway and they
  modify the infohash so you'd have to redownload their modified torrent with
  injected unique announce URL anyway. (@mathieui, fix from @jrwren)
### Fixed
- DHT makes trackers optional, so remove the warning

## [1.0] - 2009-08-25
### Added
- Add an exception to the license to allow distributions to link mktorrent to the OpenSSL library

## [0.7] - 2009-05-31
### Fixed
- Proper support for large files on certain 32bit OS's.
  Finally mktorrent properly handles files larger than 2Gb on 32bit Linux.
  Fixes for use on Windows under MinGW/Cygwin.
  
## [0.6] - 2009-05-13
### Added
- Support for multiple web seeds
### Changed
- Raised allowable piece size
- Only add the announce-list entry if there are more than one tracker.
  Thanks to Vladimir Vučićević for reporting.
- Include public domain SHA1 implementation to make the OpenSSL dependency optional.
- Lots of portability improvements. Now compiles on Linux, MinGW, OpenBSD, OSX,
  SunOS and probably many more. A big thanks to Edwin Amsler for reporting and
  testing.
- Rewrote multi threaded hashing to optimise CPU usage on multi processor machines

## [0.5] - 2009-03-01
### Added
- Support for multiple trackers
- Support for web seed (thanks to Justin Camerer)
### Changed
- Better error messages

## [0.4] - 2009-02-22
### Added
- Support for the private flag

## [0.3] - 2009-02-22
### Fixed
- Bug concerning files longer than 2^32 bytes
- Command line parameters

## [0.2] - 2009-02-22
### Added
- Support for single file torrents
- Support for long options can now be disabled at compile time

## [0.1] - 2009-02-22
Initial release
