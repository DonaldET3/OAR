# OAR
Opal File Archiver <br />
for Unix <br />
version 1

This progam archives files, recreates files from an archive, and lists files in an archive.

If the -h option is mentioned, the program simply writes the version and help to standard error and exits.

If neither -r nor -w are mentioned, the program is in list mode; the program reads an archive file from standard input and writes the list of file paths to standard output. If -r is specified, the program is in read mode; the program reads an archive file from standard input and recreates the files contained therein. If -w is specified, the program is in write mode; the program reads files from the file system and writes an archive to standard output. The options -r and -w cannot both be mentioned.

In write mode, if no files are specified in the command line, the program will read a list of paths, one per line, from standard input. If the program enters a directory that is an ancestor of itself, the progam will quit.

options <br />
a: append files to archive <br />
f: specify path to archive <br />
k: do not overwrite existing files <br />
u: only copy a file if it is newer than the existing one <br />
v: verbose

list mode options: vf <br />
read mode options: kuvf <br />
write mode options: vaf

After the list of options, there is the list of files to process. If the file is a directory, all files in that directory are processed.

In regular list mode, only the pathname is output for each file, one line for each.

In verbose list mode, the pathname, file type, size, and modification time are output. Each piece of data is on a seperate line, and each file is seperated by a blank line. If a piece of data is not available, it is not displayed. The file type is either "directory" or "regular file". The size displayed is the length of the data field in bytes. The time is in the format "year month day hour:minute".

In verbose read and write modes, the pathname is output to standard error.

The operator is warned about filenames with newlines in them when the names are listed.

<br />
 
FILE FORMAT

file header: <br />
magic string <br />
version word

file entry:
file type byte <br />
file path length word <br />
file path data <br />
modification time exists boolean <br />
modification time: year word, month byte, day byte, hour byte, minute byte, second byte <br />
if regular file, file byte length word <br />
file data <br />
more data boolean
