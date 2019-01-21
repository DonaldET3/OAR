/* Opal file archiver
 * for Unix
 * version 1
 * 32-bit
 */


/* pieces section */

#include <stdio.h>
/* getc()
 * putc()
 * puts()
 * printf()
 * scanf()
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
 */

#include <stdbool.h>
/* bool
 * true
 * false
 */

#include <stdint.h>
/* uint8_t
 * uint32_t
 */

#include <dirent.h>
/* opendir()
 * readdir()
 * closedir()
 * DIR
 * struct dirent
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
 */

#include <fcntl.h>
/* AT_FDCWD
 */


/* definitions section */

/* "OAR1-32" */
uint8_t magic[] = {0x4F, 0x41, 0x52, 0x31, 0x2D, 0x33, 0x32, 0x00};

struct options {
 char *aname;
 bool append;
 bool overwrite;
 bool update;
 bool verbose;
 char **fnames;
 size_t ind;
};

struct dir_des {
 char *path;
 size_t inl_len;
 ino_t *inl;
 struct dir_des *next;
};

struct file_md {
 uint8_t type;
 char *path;
 bool te;
 struct tm t;
 uint32_t length;
};


/* functions section */

void error(char *message)
{
 perror(message);
 exit(EXIT_FAILURE);
}

void fail(char *message)
{
 fprintf(stderr, "%s\n", message);
 exit(EXIT_FAILURE);
}

void help()
{
 char message[] = "Opal file archiver\nfor Unix\nversion 1\n32-bit\n\n"
 "options\n"
 "h: print help and exit\n"
 "w: write mode, writes an archive\n"
 "r: read mode, reads an archive\n"
 "a: append files to archive\n"
 "f: specify path to archive\n"
 "k: do not overwrite existing files\n"
 "u: only copy a file if it is newer than the existing one\n"
 "v: verbose mode\n\n"
 "If neither -w nor -r are specified, the program is in list mode.\n"
 "The mode option must be specified first.\n\n";
 fputs(message, stderr);
}

void bad_opt(int mode, char c)
{
 if(mode == 0)
  fprintf(stderr, "-%c cannot be used in list mode\n", c);
 else if(mode == 1)
  fprintf(stderr, "-%c cannot be used in write mode\n", c);
 else if(mode == -1)
  fprintf(stderr, "-%c cannot be used in read mode\n", c);
 else
  fprintf(stderr, "the -%c option caused an error\n", c);

 exit(EXIT_FAILURE);
}

void write_word(uint32_t x, FILE *fp)
{
 int i;

 for(i = 3; i >= 0; i--)
  if(putc(0xFF & (x >> (i * 8)), fp) == EOF)
   error("write word");

 return;
}

uint32_t read_word(FILE *fp)
{
 int i, c;
 uint32_t x = 0;

 for(i = 3; i >= 0; i--)
 {
  if((c = getc(fp)) == EOF)
   error("read word");
  x |= ((uint32_t)c) << (i * 8);
 }

 return x;
}

void oa_w_header(FILE *archf)
{
 /* write magic string */
 if(fwrite(magic, 1, 8, archf) != 8)
  error("write magic");

 /* write version number */
 write_word(1, archf);

 return;
}

void oa_r_header(FILE *archf)
{
 int i, c;

 /* check magic */
 for(i = 0; i < 8; i++)
 {
  if((c = getc(archf)) == EOF)
   fail("bad file");
  if(c != magic[i])
   fail("incompatible file");
 }

 /* check version */
 if(read_word(archf) != 1)
  fail("incompatible file");

 return;
}

void oa_w_time(time_t t, FILE *archf)
{
 struct tm *tms;

 if((tms = localtime(&t)) == NULL)
  error(NULL);
 write_word(tms->tm_year + 1900, archf);
 if(putc(tms->tm_mon + 1, archf) == EOF) error("write month");
 if(putc(tms->tm_mday, archf) == EOF) error("write day");
 if(putc(tms->tm_hour, archf) == EOF) error("write hour");
 if(putc(tms->tm_min, archf) == EOF) error("write minute");
 if(putc(tms->tm_sec, archf) == EOF) error("write second");

 return;
}

void oa_r_time(struct tm *t, FILE *archf)
{
 int c;

 t->tm_year = read_word(archf) - 1900;
 if((c = getc(archf)) == EOF) fail("read month");
 t->tm_mon = c - 1;
 if((c = getc(archf)) == EOF) fail("read day");
 t->tm_mday = c;
 if((c = getc(archf)) == EOF) fail("read hour");
 t->tm_hour = c;
 if((c = getc(archf)) == EOF) fail("read minute");
 t->tm_min = c;
 if((c = getc(archf)) == EOF) fail("read second");
 t->tm_sec = c;
 t->tm_isdst = -1;

 return;
}

void oa_w_rf(char *fname, struct stat *fsmd, FILE *archf)
{
 int i, c;
 off_t len, seg;
 struct tm *tms;
 FILE *dataf;

 /* open file for reading */
 if((dataf = fopen(fname, "rb")) == NULL)
 {perror(fname); return;}

 /* write file type */
 if(putc(2, archf) == EOF)
  error("write file type");

 /* write path length */
 write_word(strlen(fname), archf);

 /* write relative file path */
 for(i = 0; fname[i]; i++)
  if(putc(fname[i], archf) == EOF)
   error("write file path");

 /* write time boolean */
 if(putc(1, archf) == EOF)
  error("write time boolean");

 /* write modification time */
 oa_w_time(fsmd->st_mtime, archf);

 len = fsmd->st_size;
 while(true)
 {
  /* If the file is 2^32 bytes or longer, it must be written in multiple segments. */
  if(len > (len & 0xFFFFFFFF))
  {
   len -= 0xFFFFFFFF;
   seg = 0xFFFFFFFF;
  }
  else
  {
   seg = len;
   len = 0;
  }

  /* write segment length word */
  write_word(seg, archf);

  /* write segment data */
  while(seg--)
  {
   if((c = getc(dataf)) == EOF)
   {
    perror("length mismatch");
    exit(EXIT_FAILURE);
   }
   if(putc(c, archf) == EOF)
    error(NULL);
  }

  /* If this is the last segment, set continuation boolean to 0. */
  if(len == 0)
  {
   if(putc(0, archf) == EOF)
    error(NULL);
   fclose(dataf);
   return;
  }

  /* if this is not the last segment, set continuation boolean to 1. */
  if(putc(1, archf) == EOF)
   error(NULL);
 }
}

void oa_w_dir(char *fname, struct stat *fsmd, FILE *archf)
{
 int i;

 /* write file type */
 if(putc(1, archf) == EOF)
  error("write file type");

 /* write path length */
 write_word(strlen(fname), archf);

 /* write relative file path */
 for(i = 0; fname[i]; i++)
  if(putc(fname[i], archf) == EOF)
   error("write file path");

 /* write time boolean */
 if(putc(1, archf) == EOF)
  error("time boolean");

 /* write modification time */
 oa_w_time(fsmd->st_mtime, archf);

 return;
}

/* start a directory list */
struct dir_des * dl_start(char *path, ino_t inode_num)
{
 struct dir_des *f_dir, *caboose;

 if((f_dir = malloc(sizeof(struct dir_des))) == NULL) error(NULL);
 f_dir->path = strcpy(malloc(strlen(path) + 1), path);
 f_dir->inl_len = 1;
 f_dir->inl = malloc(sizeof(ino_t));
 f_dir->inl[0] = inode_num;
 caboose = malloc(sizeof(struct dir_des));
 caboose->path = NULL;
 caboose->inl_len = 0;
 caboose->inl = NULL;
 caboose->next = NULL;
 f_dir->next = caboose;

 return f_dir;
}

/* add a directory to the list */
struct dir_des * dl_add(struct dir_des *l_dir, char *path, struct dir_des *p_dir, ino_t inode_num)
{
 int i;
 struct dir_des *caboose;

 l_dir->path = strcpy(malloc(strlen(path) + 1), path);
 l_dir->inl_len = p_dir->inl_len + 1;
 l_dir->inl = malloc(l_dir->inl_len * sizeof(ino_t));
 for(i = 0; i < p_dir->inl_len; i++)
  l_dir->inl[i] = p_dir->inl[i];
 l_dir->inl[i] = inode_num;
 caboose = malloc(sizeof(struct dir_des));
 caboose->path = NULL;
 caboose->inl_len = 0;
 caboose->inl = NULL;
 caboose->next = NULL;
 l_dir->next = caboose;

 return caboose;
}

/* remove a directory from the list */
struct dir_des * dl_remove(struct dir_des *c_dir)
{
 struct dir_des *n_dir;

 free(c_dir->path);
 free(c_dir->inl);
 n_dir = c_dir->next;
 free(c_dir);

 return n_dir;
}

/* process directory */
void oa_proc_dir(char *fname, struct stat *rdir_md, FILE *archf, bool verbose)
{
 int i;
 char *fpath = NULL;
 struct dir_des *c_dir, *l_dir, *caboose;
 struct dirent *dir_e;
 struct stat fsmd;
 DIR *dir;

 /* write the directory */
 if(verbose)
  fprintf(stderr, "%s\n", fname);
 oa_w_dir(fname, rdir_md, archf);

 /* initialize the directory list */
 c_dir = dl_start(fname, rdir_md->st_ino);
 caboose = c_dir->next;

 /* process the files inside */
 while(c_dir != caboose)
 {
  if((dir = opendir(c_dir->path)) == NULL)
  {perror(c_dir->path); continue;}

  while((dir_e = readdir(dir)) != NULL)
  {
   /* ignore the current and parent directories */
   if((!strcmp(dir_e->d_name, ".")) || (!strcmp(dir_e->d_name, "..")))
    continue;

   /* put together the path to the file currently being processed */
   if((fpath = realloc(fpath, strlen(c_dir->path) + strlen(dir_e->d_name) + 2)) == NULL) error(NULL);
   sprintf(fpath, "%s/%s", c_dir->path, dir_e->d_name);

   if(stat(fpath, &fsmd) == -1)
   {perror(fpath); continue;}

   /* write a regular file */
   if(S_ISREG(fsmd.st_mode))
   {
    if(verbose)
     fprintf(stderr, "%s\n", fpath);
    oa_w_rf(fpath, &fsmd, archf);
   }
   /* write a directory and add it to the list for further processing */
   else if(S_ISDIR(fsmd.st_mode))
   {
    for(i = 0; i < c_dir->inl_len; i++)
     if(c_dir->inl[i] == dir_e->d_ino)
      fail("infinite hardlink loop");
    if(verbose)
     fprintf(stderr, "%s\n", fpath);
    oa_w_dir(fpath, &fsmd, archf);
    caboose = dl_add(caboose, fpath, c_dir, dir_e->d_ino);
   }
   else
    fprintf(stderr, "skipping: %s (not a regular file nor a directory)\n", fname);
  }
  c_dir = dl_remove(c_dir);
 }

 free(c_dir);
 free(fpath);

 return;
}

void get_fnames(struct options *opts)
{
 int c = '\0';
 char *fname, **fnames;
 size_t pos, len, name_num = 0;

 /* prepare the filename array */
 if((fnames = malloc(sizeof(char *))) == NULL) error(NULL);
 fnames[0] = NULL;

 while(c != EOF)
 {
  /* find the start of a filename */
  if((c = getchar()) == EOF)
   break;
  if((c == '\n') || (c == '\0'))
   continue;

  /* read the filename */
  pos = 0; len = 127;
  if((fname = malloc(128)) == NULL) error(NULL);
  while((c != EOF) && (c != '\n') && (c != '\0'))
  {
   fname[pos++] = c;
   if(pos == len)
    fname = realloc(fname, (len += 128) + 1);
   c = getchar();
  }

  /* terminate the filename */
  fname[pos] = '\0';

  /* add the filename to the list */
  if((fnames = realloc(fnames, (++name_num + 1) * sizeof(char *))) == NULL)
   error(NULL);
  fnames[name_num - 1] = fname;
  fnames[name_num] = NULL;
 }

 opts->fnames = fnames;
 opts->ind = 0;

 return;
}

bool oa_r_fmd(struct file_md *fmd, FILE *archf)
{
 int c, i;
 size_t p_len;

 /* read file record type */
 if((c = getc(archf)) == EOF)
  return false;
 fmd->type = c;

 /* read path length */
 p_len = read_word(archf);
 fmd->path = realloc(fmd->path, p_len + 1);

 /* read path */
 for(i = 0; i < p_len; i++)
 {
  if((c = getc(archf)) == EOF)
   fail("read path");
  fmd->path[i] = c;
 }
 fmd->path[i] = '\0';

 /* read time boolean */
 if((c = getc(archf)) == EOF)
  fail("read time boolean");
 fmd->te = c;

 /* read modification time */
 if(fmd->te)
  oa_r_time(&fmd->t, archf);

 return true;
}

void oa_data_length(struct file_md *fmd, FILE *archf)
{
 int c;
 size_t seg;

 fmd->length = 0;

 while(true)
 {
  /* read segment length */
  seg = read_word(archf);
  fmd->length += seg;

  /* skip segment data */
  while(seg--)
   if((c = getc(archf)) == EOF)
    error("read record data");

  /* continuation boolean */
  if((c = getc(archf)) == EOF)
   error("read record data");

  if(!c)
   return;
 }
}

bool list_match(char *arg, char *path)
{
 int i;

 /* a single dot matches any file in the root directory */
 if(!strcmp(arg, "."))
 {
  if(strstr(path, "/") == NULL)
   return true;
  else
   return false;
 }

 /* check how much of the path is the same */
 for(i = 0; (arg[i] == path[i]) && arg[i] && path[i]; i++);

 /* check whether path is longer than arg */
 if(arg[i] || (path[i] == '\0'))
  return false;

 /* if the last character in arg was not a slash, advance */
 if(arg[i - 1] != '/')
  if(path[++i] == '\0')
   return false;

 /* check whether path is a child of arg */
 if(path[i - 1] != '/')
  return false;

 /* path cannot be more than one level down the heirarchy */
 if(strstr(path + i, "/") != NULL)
  return false;

 return true;
}

void w_listing(struct file_md *fmd, bool verbose)
{
 char dates[128];

 /* verbose listing */
 if(verbose)
 {
  /* file path */
  printf("relative path: %s\n", fmd->path);
  /* file type (and size) */
  if(fmd->type == 2)
  {
   puts("type: regular file");
   printf("size: %lu bytes\n", (unsigned long)fmd->length);
  }
  else if(fmd->type == 1)
   puts("type: directory");
  else
   puts("type: unrecognized");
  /* file modification time */
  if(fmd->te)
  {
   strftime(dates, 128, "%Y %b %e %H:%M", &fmd->t);
   printf("last modified: %s\n", dates);
  }
  putchar('\n');
 }
 /* terse lisitng */
 else
  puts(fmd->path);

 /* line feed warning */
 if(strstr(fmd->path, "\n") != NULL)
  puts("The last file path contains line feed characters!\n");

 return;
}

bool read_match(char *arg, char *path)
{
 int i;

 /* a single dot matches any file */
 if(!strcmp(arg, "."))
  return true;

 /* check how much of the path is the same */
 for(i = 0; (arg[i] == path[i]) && arg[i] && path[i]; i++);

 /* if arg is longer than path, path must be a directory */
 if(arg[i])
 {
  if((arg[i] == '/') && (path[i] == '\0'))
   return true;
  else
   return false;
 }

 /* check whether path is a child of arg */
 if(arg[i - 1] != '/')
  if((path[i] != '/') && (path[i] != '\0'))
   return false;

 return true;
}

bool criteria(struct file_md *fmd, struct options *opts)
{
 struct stat fsmd;

 /* If there is no file by that name, the path is free. */
 if(stat(fmd->path, &fsmd) == -1)
  return true;

 /* If the program is set to never overwrite, it fails. */
 if(!opts->overwrite)
  return false;

 /* If the archived file is not newer than the existing file, it fails. */
 if(opts->update)
  if(mktime(&fmd->t) <= fsmd.st_mtime)
   return false;

 return true;
}

void oa_r_dir(struct file_md *fmd)
{
 struct stat fsmd;
 struct timespec etime[2];

 /* If there is already a file, don't create a new directory. */
 if(stat(fmd->path, &fsmd) == 0)
 {
  if(!(S_ISDIR(fsmd.st_mode)))
   fprintf(stderr, "%s: not a directory\n", fmd->path);
  return;
 }

 /* create directory */
 if(mkdir(fmd->path, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
 {perror(NULL); return;}

 /* restore old modification time */
 if(fmd->te)
 {
  etime[0].tv_sec = 0;
  etime[0].tv_nsec = UTIME_OMIT;
  etime[1].tv_sec = mktime(&fmd->t);
  etime[1].tv_nsec = 0;
  utimensat(AT_FDCWD, fmd->path, etime, 0);
 }

 return;
}

void oa_r_rf(struct file_md *fmd, FILE *archf)
{
 int i, c;
 off_t len = 0;
 struct timespec etime[2];
 FILE *dataf;

 if((dataf = fopen(fmd->path, "wb")) == NULL)
 {
  perror(fmd->path);
  oa_data_length(fmd, archf);
  return;
 }

 while(true)
 {
  /* read segment length word */
  len = read_word(archf);

  /* read segment data */
  while(len)
  {
   if((c = getc(archf)) == EOF)
    fail("incomplete file");
   if(putc(c, dataf) == EOF)
    error(NULL);
   len--;
  }

  /* read continuation boolean */
  if((c = getc(archf)) == EOF)
   fail("incomplete file");
  if(!c)
   break;
 }

 fclose(dataf);

 /* set modification time */
 if(fmd->te)
 {
  etime[0].tv_sec = 0;
  etime[0].tv_nsec = UTIME_OMIT;
  etime[1].tv_sec = mktime(&fmd->t);
  etime[1].tv_nsec = 0;
  utimensat(AT_FDCWD, fmd->path, etime, 0);
 }

 return;
}

/* write archive */
void oa_write(struct options *opts)
{
 char *fname;
 struct stat fsmd;
 FILE *archf;

 /* setup archive file */
 if(opts->append)
 {
  if((archf = fopen(opts->aname, "ab")) == NULL)
   error(opts->aname);
 }
 else
 {
  if(opts->aname == NULL)
   archf = stdout;
  else
  {
   if((archf = fopen(opts->aname, "wb")) == NULL)
    error(opts->aname);
  }
  oa_w_header(archf);
 }

 /* If files are not specified on the command line,
    they must be specified through standard input. */
 if(opts->fnames[opts->ind] == NULL)
  get_fnames(opts);

 /* write content files */
 for(; (fname = opts->fnames[opts->ind]) != NULL; opts->ind++)
 {
  if(stat(fname, &fsmd) == -1)
  {
   perror(fname);
   continue;
  }

  if(S_ISREG(fsmd.st_mode))
  {
   if(opts->verbose)
    fprintf(stderr, "%s\n", fname);
   oa_w_rf(fname, &fsmd, archf);
  }
  else if(S_ISDIR(fsmd.st_mode))
   oa_proc_dir(fname, &fsmd, archf, opts->verbose);
  else
   fprintf(stderr, "skipping: %s (not a regular file nor a directory)\n", fname);
 }

 if(opts->aname != NULL)
  fclose(archf);

 return;
}

/* list archive members */
void oa_list(struct options *opts)
{
 size_t i;
 struct file_md fmd;
 FILE *archf;

 fmd.path = NULL;

 if(opts->aname == NULL)
  archf = stdin;
 else
 {
  if((archf = fopen(opts->aname, "rb")) == NULL)
   error(opts->aname);
 }
 oa_r_header(archf);

 /* for each file in the archive... */
 while(oa_r_fmd(&fmd, archf))
 {
  /* find the length of the data */
  if(fmd.type == 2)
   oa_data_length(&fmd, archf);

  /* for each path argument... */
  for(i = opts->ind; opts->fnames[i] != NULL; i++)
   /* check whether the file in inside the directory */
   if(list_match(opts->fnames[i], fmd.path))
   {
    /* write the listing */
    w_listing(&fmd, opts->verbose);
    break;
   }
 }

 if(opts->aname != NULL)
  fclose(archf);

 return;
}

/* read archive */
void oa_read(struct options *opts)
{
 size_t i;
 struct file_md fmd;
 FILE *archf;

 fmd.path = NULL;

 if(opts->aname == NULL)
  archf = stdin;
 else
 {
  if((archf = fopen(opts->aname, "rb")) == NULL)
   error(opts->aname);
 }
 oa_r_header(archf);

 /* for each file in the archive... */
 while(oa_r_fmd(&fmd, archf))
 {
  /* for each path argument... */
  for(i = opts->ind; opts->fnames[i] != NULL; i++)
   if(read_match(opts->fnames[i], fmd.path))
    /* check whether the path meets the specified criteria */
    if(criteria(&fmd, opts))
    {
     if(opts->verbose)
      fprintf(stderr, "%s\n", fmd.path);

     if(fmd.type == 1)
      oa_r_dir(&fmd);
     else if(fmd.type == 2)
      oa_r_rf(&fmd, archf);
     break;
    }

  if((opts->fnames[i] == NULL) && (fmd.type == 2))
   oa_data_length(&fmd, archf);
 }

 if(opts->aname != NULL)
  fclose(archf);

 return;
}

int main(int argc, char **argv)
{
 int c, mode = 0;
 struct options opts;
 extern char *optarg;
 extern int opterr, optind, optopt;

 opts.aname = NULL;
 opts.append = false; opts.overwrite = true;
 opts.update = false; opts.verbose = false;

 /* parse command line */
 while((c = getopt(argc, argv, "hrwaf:kuv")) != -1)
  switch(c)
  {
   case 'h': help(); exit(EXIT_SUCCESS);
   case 'w': if(mode > -1) mode = 1; else bad_opt(mode, c); break;
   case 'r': if(mode < 1) mode = -1; else bad_opt(mode, c); break;
   case 'a': if(mode > 0) opts.append = true; else bad_opt(mode, c); break;
   case 'f': opts.aname = optarg; break;
   case 'k': if(mode < 0) opts.overwrite = false; else bad_opt(mode, c); break;
   case 'u': if(mode < 0) opts.update = true; else bad_opt(mode, c); break;
   case 'v': opts.verbose = true; break;
   case '?': exit(EXIT_FAILURE);
  }

 if(opts.append && (opts.aname == NULL))
  error("When appending, an archive file must be specified.");

 opts.fnames = argv;
 opts.ind = optind;

 if(mode == 1)
  oa_write(&opts);
 else if(mode == 0)
  oa_list(&opts);
 else if(mode == -1)
  oa_read(&opts);
 else
  return EXIT_FAILURE;

 return EXIT_SUCCESS;
}
