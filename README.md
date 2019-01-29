# Opal File Archiver (OAR)
for Unix, version 1

You should review the source code of this program before using it with important files or a system that contains important files. You take all responsibility for what happens when you use this program. If you do decide to use this program, save a copy of the source code; the code in this repository may be replaced by an entirely incompatible program at any time.

This progam archives files, recreates files from an archive, and lists files in an archive.

If the -h option is mentioned, the program simply writes the version and help to standard error and exits.

If neither -r nor -w are mentioned, the program is in list mode; the program reads an archive file from standard input and writes the list of file paths to standard output. If -r is specified, the program is in read mode; the program reads an archive file from standard input and recreates the files contained therein. If -w is specified, the program is in write mode; the program reads files from the file system and writes an archive to standard output. The options -r and -w cannot both be mentioned.

In write mode, if no files are specified in the command line, the program will read a list of paths, one per line, from standard input. If the program enters a directory that is an ancestor of itself, the progam will quit.

### options  
a: append files to archive  
f: specify path to archive  
k: do not overwrite existing files  
u: only copy a file if it is newer than the existing one  
v: verbose

list mode options: v, f  
read mode options: k, u, v, f  
write mode options: v, a, f

After the list of options, there is the list of files to process. If the file is a directory, all files in that directory are processed.

In regular list mode, only the pathname is output for each file, one line for each.

In verbose list mode, the pathname, file type, size, and modification time are output. Each piece of data is on a seperate line, and each file is seperated by a blank line. If a piece of data is not available, it is not displayed. The file type is either "directory" or "regular file". The size displayed is the length of the data field in bytes. The time is in the format "year month day hour:minute".

In verbose read and write modes, the pathname is output to standard error.

The operator is warned about filenames with newlines in them when the names are listed.

_______

## FILE FORMAT

### file header  
magic string  
version word

### file entry  
file type byte  
file path length word  
file path data  
modification time exists boolean  
modification time: year word, month byte, day byte, hour byte, minute byte, second byte  
if regular file, file byte length word  
file data  
more data boolean
