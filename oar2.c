/* Opal file archiver
 * for Unix
 * version 2
 */


/* pieces section */

#include <errno.h>
/* errno
 */

#include <stdio.h>
/* getchar()
 * putchar()
 * getc()
 * putc()
 * fputs()
 * printf()
 * scanf()
 * getline()
 * getdelim()
 * fopen()
 * fclose()
 * perror()
 * FILE
 * NULL
 * EOF
 */

#include <stdlib.h>
/* exit()
 * NULL
 * EXIT_FAILURE
 * EXIT_SUCCESS
 */

#include <string.h>
/* strlen()
 * strcpy()
 * strstr()
 * strerror_l()
 */

#include <stdbool.h>
/* bool
 * true
 * false
 */

#include <stdint.h>
/* uint8_t
 * uintmax_t
 */

#include <dirent.h>
/* opendir()
 * readdir()
 * closedir()
 * DIR
 * struct dirent
 */

#include <locale.h>
/* uselocale()
 */

#include <unistd.h>
/* getopt()
 */

#include <time.h>
/* strftime()
 * localtime()
 * mktime()
 * struct tm
 * struct timespec
 */

#include <sys/stat.h>
/* stat()
 * mkdir()
 * utimensat()
 * S_ISREG()
 * S_ISDIR()
 * struct stat
 * UTIME_OMIT
 */

#include <fcntl.h>
/* AT_FDCWD
 */


/* definitions section */

/* "OAR2" */
uint8_t magic[] = {0x4F, 0x41, 0x52, 0x32, 0x00};

/* program options */
struct options {
 /* subset path */
 char *subset;
 /* verbose mode */
 bool verbose;
};

/* file metadata */
struct file_md {
 /* file type */
 char type;
 /* file path */
 char *path;
 /* path space size */
 size_t space;
 /* does time information exist for the file? */
 int te;
 /* time structure */
 struct tm t;
 /* file data length */
 uintmax_t length;
};

/* text input buffer */
char *i_buf;
/* buffer size */
size_t ib_s;


/* functions section */

/* print error message and quit */
void fail(char *message)
{
 /* print error message */
 fputs(message, stderr);
 /* elaborate on the error if possible */
 if(errno) fprintf(stderr, ": %s", strerror_l(errno, uselocale((locale_t)0)));
 putc('\n', stderr);
 exit(EXIT_FAILURE);
}

/* "failed to" <error message> and quit */
void failed(char *message)
{
 /* prepend "failed to" to the error message */
 fputs("failed to ", stderr);
 fail(message);
}

/* help message */
void help()
{
 char message[] = "Opal file archiver\n"
 "version 2\n\n"
 "options\n"
 "h: print help and exit\n"
 "w: write mode, writes an archive\n"
 "r: read mode, reads an archive\n"
 "p: specify a subset path of archive\n"
 "v: verbose mode\n\n"
 "If neither -w nor -r are specified, the program is in list mode.\n"
 "The mode option must be specified first.\n\n";
 fputs(message, stderr);
}

/* bad option from command line */
void bad_opt(int mode, char c)
{
 if(mode == 0) fprintf(stderr, "-%c cannot be used in list mode\n", c);
 else if(mode == 1) fprintf(stderr, "-%c cannot be used in write mode\n", c);
 else if(mode == -1) fprintf(stderr, "-%c cannot be used in read mode\n", c);
 else fprintf(stderr, "the -%c option caused an error\n", c);
 exit(EXIT_FAILURE);
}

/* set modification time */
void set_mod_time(struct file_md *fmd)
{
 struct timespec etime[2];

 if(fmd->te)
 {
  etime[0].tv_sec = 0;
  etime[0].tv_nsec = UTIME_OMIT;
  etime[1].tv_sec = mktime(&fmd->t);
  etime[1].tv_nsec = 0;
  utimensat(AT_FDCWD, fmd->path, etime, 0);
 }
}

/* write hexadecimal number */
void write_number(uintmax_t x)
{
 if(printf("%jX", x) < 0) failed("write number value");
 if(putchar('\0') == EOF) failed("write number terminator");
 return;
}

/* read hexadecimal number */
uintmax_t read_number()
{
 uintmax_t x;
 if(getdelim(&i_buf, &ib_s, '\0', stdin) == -1) failed("read number");
 if(sscanf(i_buf, "%jx", &x) != 1) failed("comprehend number");
 return x;
}

/* write opal archive header */
void oa_w_header()
{
 /* write magic string */
 if(fwrite(magic, 1, 5, stdout) != 5) failed("write magic");

 /* write version number */
 putchar(1);

 return;
}

/* read opal archive header */
void oa_r_header()
{
 int i;

 /* check magic */
 for(i = 0; i < 5; i++)
  if(getchar() != magic[i])
   fail("unrecognized file type");

 /* check version */
 if(getchar() != 1)
  fail("incompatible archive version");

 return;
}

/* write opal archive time */
void oa_w_time(time_t t)
{
 struct tm tms;

 if((localtime_r(&t, &tms)) == NULL) failed("convert time");
 write_number(tms.tm_year + 1900);
 write_number(tms.tm_mon + 1);
 write_number(tms.tm_mday);
 write_number(tms.tm_hour);
 write_number(tms.tm_min);
 write_number(tms.tm_sec);

 return;
}

/* read opal archive time */
void oa_r_time(struct tm *t)
{
 t->tm_year = read_number() - 1900;
 t->tm_mon = read_number() - 1;
 t->tm_mday = read_number();
 t->tm_hour = read_number();
 t->tm_min = read_number();
 t->tm_sec = read_number();
 t->tm_isdst = -1;
 return;
}

/* read file metadata from opal archive */
bool oa_r_fmd(struct file_md *fmd)
{
 int c;

 /* read file record type */
 if((c = getchar()) == EOF) return false;
 fmd->type = c;

 /* read path */
 if(getdelim(&fmd->path, &fmd->space, '\0', stdin) == -1) failed("read path");

 /* read time boolean */
 if((c = getchar()) == EOF) failed("read time boolean");
 fmd->te = c;

 /* read modification time */
 if(fmd->te) oa_r_time(&fmd->t);

 /* read file data length */
 if(fmd->type == 'r') fmd->length = read_number();

 return true;
}

/* skip the file data in a regular file record */
void oa_skip_data(struct file_md *fmd)
{
 size_t i;

 for(i = 0; i < fmd->length; i++) if(getchar() == EOF) fail("incomplete file record");

 return;
}

/* write an opal archive directory record */
void oa_w_dir(char *f_name, struct stat *fsmd)
{
 /* write file type */
 if(putchar('d') == EOF) failed("write file type");

 /* write relative file path */
 if(fputs(f_name, stdout) == EOF) failed("write file path");
 if(putchar('\0') == EOF) failed("write path terminator");

 /* write time boolean */
 if(putchar(1) == EOF) failed("write time boolean");

 /* write modification time */
 oa_w_time(fsmd->st_mtime);

 return;
}

/* read a directory record from an opal archive */
void oa_r_dir(struct file_md *fmd)
{
 struct stat fsmd;

 /* If there is already a file, don't create a new directory. */
 if(stat(fmd->path, &fsmd) == 0)
 {
  /* ensure the file is a directory */
  if(!(S_ISDIR(fsmd.st_mode))) fprintf(stderr, "%s: not a directory\n", fmd->path);
  return;
 }

 /* create directory */
 if(mkdir(fmd->path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
 {perror(fmd->path); return;}

 /* restore old modification time */
 if(fmd->te) set_mod_time(fmd);

 return;
}

/* write an opal archive regular file record */
void oa_w_rf(char *f_name, struct stat *fsmd)
{
 int c;
 off_t i;
 FILE *data_f;

 /* open file for reading */
 if((data_f = fopen(f_name, "rb")) == NULL)
 {perror(f_name); return;}

 /* write file type */
 if(putchar('r') == EOF) failed("write file type");

 /* write relative file path */
 if(fputs(f_name, stdout) == EOF) failed("write file path");
 if(putchar('\0') == EOF) failed("write path terminator");

 /* write time boolean */
 if(putchar(1) == EOF) failed("write time boolean");

 /* write modification time */
 oa_w_time(fsmd->st_mtime);

 /* write data length */
 write_number(fsmd->st_size);

 /* write file data */
 for(i = 0; i < fsmd->st_size; i++)
 {
  if((c = getc(data_f)) == EOF) fail(f_name);
  if(putchar(c) == EOF) failed("write file data");
 }

 return;
}

/* read a regular file record from an opal archive */
void oa_r_rf(struct file_md *fmd)
{
 int c;
 off_t i;
 FILE *data_f;

 /* open data file */
 if((data_f = fopen(fmd->path, "wb")) == NULL)
 {
  perror(fmd->path);
  oa_skip_data(fmd);
  return;
 }

 /* restore file data */
 for(i = 0; i < fmd->length; i++)
 {
  if((c = getchar()) == EOF) fail("incomplete file");
  if(putc(c, data_f) == EOF) fail(fmd->path);
 }

 /* close data file */
 fclose(data_f);

 /* restore old modification time */
 if(fmd->te) set_mod_time(fmd);

 return;
}

/* find whether parent is a parent of child */
char * is_parent(char *parent, char *child)
{
 size_t par_len;

 /* if child is longer than parent,
  * parent is at the start of child,
  * and followed by a slash in child,
  * it is a match */
 if(strlen(child) > (par_len = strlen(parent)))
  if(strstr(child, parent) == child)
   if(child[par_len] == '/')
    return child + par_len + 1;
 
 return NULL;
}

/* write file listing */
void w_listing(struct file_md *fmd, bool verbose)
{
 char dates[128];

 /* verbose listing */
 if(verbose)
 {
  /* file path */
  printf("relative path: %s\n", fmd->path);

  /* file type (and size) */
  if(fmd->type == 'r')
  {
   puts("type: regular file");
   printf("size: %ju bytes\n", fmd->length);
  }
  else if(fmd->type == 'd') puts("type: directory");
  else puts("type: unrecognized");

  /* file modification time */
  if(fmd->te)
  {
   strftime(dates, 128, "%Y %b %e %H:%M", &fmd->t);
   printf("last modified: %s\n", dates);
  }

  putchar('\n');
 }

 /* terse lisitng */
 else puts(fmd->path);

 return;
}

/* find whether path matches the argument in list mode */
bool list_match(char *arg, char *path)
{
 char *f_name;

 /* a single dot matches any file in the root directory */
 if(!strcmp(arg, "."))
 {
  if(strstr(path, "/") == NULL) return true;
  return false;
 }

 /* check whether arg is a parent of path */
 if((f_name = is_parent(arg, path)) == NULL) return false;
 /* file cannot be more than one level down the heirarchy */
 if(strstr(f_name, "/") != NULL) return false;
 return true;
}

/* find whether the path matches the argument in read mode */
bool read_match(char *arg, char *path)
{
 /* NULL arg matches any file */
 if(arg == NULL) return true;
 /* a single dot matches any file */
 if(!strcmp(arg, ".")) return true;

 /* arg matches itself */
 if(!strcmp(arg, path)) return true;

 /* if path is a parent of arg, it is a match */
 if(is_parent(path, arg) != NULL) return true;
 /* if arg is a parent of path, it is a match */
 return is_parent(arg, path) != NULL;
}

/* write archive */
void oa_write(struct options *opts)
{
 char *line = NULL, *f_name;
 size_t size = 0;
 struct stat fsmd;

 /* write archive file header */
 oa_w_header();

 /* for each filename from standard input... */
 while(getline(&line, &size, stdin) != -1)
 {
  /* trim newline off the end and get file metadata */
  if(stat(f_name = strtok(line, "\n"), &fsmd) == -1)
  {
   perror(f_name);
   /* next filename */
   continue;
  }

  /* verbose mode */
  if(opts->verbose) fprintf(stderr, "%s\n", f_name);

  /* process regular file */
  if(S_ISREG(fsmd.st_mode)) oa_w_rf(f_name, &fsmd);
  /* processs directory */
  else if(S_ISDIR(fsmd.st_mode)) oa_w_dir(f_name, &fsmd);
  /* unsupported file type */
  else fprintf(stderr, "skipping: %s (not a regular file nor a directory)\n", f_name);
 }

 /* free filename line space */
 free(line);

 return;
}

/* list archive members */
void oa_list(struct options *opts)
{
 struct file_md fmd;

 /* initialize dynamic string space */
 fmd.path = NULL;
 fmd.space = 0;

 /* a path must be specified */
 if(opts->subset == NULL) fail("no path specified");

 /* read archive file header */
 oa_r_header();

 /* for each file in the archive... */
 while(oa_r_fmd(&fmd))
 {
  /* write the listing if the file is in inside the directory */
  if(list_match(opts->subset, fmd.path)) w_listing(&fmd, opts->verbose);

  /* skip the data of the file record */
  if(fmd.type == 'r') oa_skip_data(&fmd);
 }

 /* free dynamic string space */
 free(fmd.path);

 return;
}

/* read archive */
void oa_read(struct options *opts)
{
 struct file_md fmd;

 /* initialize dynamic string space */
 fmd.path = NULL;
 fmd.space = 0;

 /* read archive file header */
 oa_r_header();

 /* for each file in the archive... */
 while(oa_r_fmd(&fmd))
 {
  /* compare the path */
  if(read_match(opts->subset, fmd.path))
  {
   /* verbose mode */
   if(opts->verbose) fprintf(stderr, "%s\n", fmd.path);

   /* process directory */
   if(fmd.type == 'd') oa_r_dir(&fmd);
   /* process regular file */
   else if(fmd.type == 'r') oa_r_rf(&fmd);
   /* unrecognized record type */
   else fail("unrecognized record type");
  }
  else
  {
   /* skip the data of the file record */
   if(fmd.type == 'r') oa_skip_data(&fmd);
  }
 }

 /* free dynamic string space */
 free(fmd.path);

 return;
}

int main(int argc, char **argv)
{
 int c, mode = 0;
 struct options opts;
 extern char *optarg;
 extern int opterr, optind, optopt;

 /* initialize options structure */
 opts.subset = NULL;
 opts.verbose = false;

 /* initialize global variables */
 i_buf = NULL;
 ib_s = 0;

 /* the errno symbol is defined in errno.h */
 errno = 0;

 /* parse command line */
 while((c = getopt(argc, argv, "hwrp:v")) != -1)
  switch(c)
  {
   case 'h': help(); exit(EXIT_SUCCESS);
   case 'w': if(mode > -1) mode = 1; else bad_opt(mode, c); break;
   case 'r': if(mode < 1) mode = -1; else bad_opt(mode, c); break;
   case 'p': opts.subset = optarg; break;
   case 'v': opts.verbose = true; break;
   case '?': exit(EXIT_FAILURE);
  }

 /* process archive */
 if(mode == 1) oa_write(&opts);
 else if(mode == 0) oa_list(&opts);
 else if(mode == -1) oa_read(&opts);
 else return EXIT_FAILURE;

 free(i_buf);

 return EXIT_SUCCESS;
}
