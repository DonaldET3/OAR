Opal file archiver
for Unix
version 2.1

This progam archives files, recreates files from an archive, and lists files in
an archive.

If neither -r nor -w are mentioned, the program is in list mode; the program
reads an archive file from standard input and writes the list of file paths to
standard output. If -r is specified, the program is in read mode; the program
reads an archive file from standard input and recreates the files contained
therein. If -w is specified, the program is in write mode; the program reads
files from the file system and writes an archive to standard output. The
options -r and -w cannot both be mentioned.

In write mode, the program reads a list of paths, one per line, from standard
input.

__options__
h: print help and exit
w: write mode
r: read mode
p: subset path of archive to read
k: do not overwrite existing files
u: only restore a file if it is newer than the existing one
v: verbose

In regular list mode, only the pathname is output for each file, one line for
each.

In verbose list mode, the pathname, file type, size, and modification time are
output. Each piece of data is on a seperate line, and each file is seperated by
a blank line. If a piece of data is not available, it is not displayed. The
file type is either "directory" or "regular file". The size displayed is the
length of the data field in bytes. The time is in the format
"year month day hour:minute".

In verbose read and write modes, the pathname is output to standard error.


FILE FORMAT

__file header__
magic string
version byte

__file entry__
file type byte
file path string
modification time exists boolean
modification time (hexadecimal strings): year, month, day, hour, minute, second
if regular file, file byte length hexadecimal string
file data

Directories must be recorded before being used in a file path.
