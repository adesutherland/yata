/***********************************************************************/
/*                                                                     */
/* SPLTCON.C                                                           */
/*                                                                     */
/* CONCAT [d] - Combines file on d drive (default a) into SPLIT CONCAT */
/* SPLIT [d] - Splits files from SPLIT CONCAT A in to seperata files   */
/*             on d drive (default A)                                  */
/*                                                                     */
/***********************************************************************/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define MAXRECL 800
 
#ifdef __CMS
 
#define HELPTEXT "\nSPLIT|CONCAT [D], where D (default=A) is the disk\n"
#define SPLIT "SPLIT"
#define CONCAT "CONCAT"
 
#include <cmssys.h>
#define CONCATFILE "SPLIT CONCAT A1"
char* includeTypes[] = { "C", "H", "EXEC", "ASSEMBLE", "LISTING", "COPY", "MACLIB",
                         "MACRO", "PARM"};
int toupper(int c);
int tolower(int c);
 
#else
 
#define HELPTEXT "\nsplit|concat [D], where D (default is . [i.e. current]) is the directory\n"
 
#ifdef _WIN32
 
#include <windows.h>
 
#else
 
#include <dirent.h>
#include <libgen.h>
 
#endif
 
#define SPLIT "split"
#define CONCAT "concat"
 
#include <ctype.h>
#define CONCATFILE "spltcon.txt"
char* includeTypes[] = { "c", "h", "exec", "assemble", "listing", "copy", "maclib",
                         "macro", "parm"};
 
#endif
 
int concat(void);
int split(void);
char* validateFileName(char *listFileLine);
char* toStoredName(char *fileName);
char* fromStoredName(char *fileName);
 
char* drive;
 
#ifdef _WIN32
char* basename(char* path)
{
    char* base = strrchr(path, '\\');
    return base ? base + 1 : path;
}
#endif
 
int main(int argc, char * argv[]) {
  char* name;
  int error = 0;
 
#ifdef __CMS
  name = argv[0];
#else
  name = basename(argv[0]);
  name = strtok(name, ".");
#endif
 
  if (argc>2) {
    error = 1;
    printf("ERROR: Too many arguments\n");
  }
  if (argc==2) {
#ifdef __CMS
    if (strlen(argv[1])!=1) {
      error = 1;
      printf("ERROR: Invalid drive\n");
    }
#endif
    drive = argv[1];
  }
  else {
#ifdef __CMS
    drive = "A";
#else
    drive = ".";
#endif
  }
 
  if (!strcmp(name,SPLIT)) {
    if (!error) return split();
  }
  else if (!strcmp(name,CONCAT)) {
    if (!error) return concat();
  }
  else printf("ERROR: Module name must be SPLIT or CONCAT - not %s\n",name);
  printf(HELPTEXT);
  return -1;
}
 
int concat(void) {
#ifdef __CMS
  int stackSize;
  int files;
  char consoleLine[131];
#else
#ifdef _WIN32
  WIN32_FIND_DATA fdFile;
  HANDLE hFind = NULL;
#else
  DIR *d;
  struct dirent *dir;
#endif
#endif
  char* fileName;
  FILE* inFile;
  FILE* outFile;
  char lineBuffer[MAXRECL];
 
  outFile = fopen(CONCATFILE, "w");
 
#ifdef __CMS
  int i;
  char command[40];
  stackSize = CMSstackQuery();
  sprintf(command, "LISTFILE * * %s (FORMAT STACK", drive);
  CMScommand(command, CMS_COMMAND);
  files = CMSstackQuery() - stackSize;
  for (i=0; i<files; i++) {
    if (CMSconsoleRead(consoleLine)) {
      fileName = validateFileName(consoleLine);
#else
 
#ifdef _WIN32
 
 char dirAndName[260];
 snprintf(dirAndName, 259, "%s\\*.*", drive);
 hFind = FindFirstFile(dirAndName, &fdFile);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      snprintf(dirAndName, 259, "%s\\%s", drive, fdFile.cFileName);
      fileName = validateFileName(dirAndName);
 
#else
  d = opendir(drive);
  if (d)  {
    char dirAndName[260];
    while ((dir = readdir(d)) != NULL) {
      snprintf(dirAndName, 259, "%s/%s", drive, dir->d_name);
      fileName = validateFileName(dirAndName);
 
#endif
#endif
 
      if (fileName) {
        inFile = fopen(fileName,"r");
        if (inFile==NULL) {
          return -1;
        }
        fprintf(outFile, "+%s\n", toStoredName(fileName));
 
        while (fgets(lineBuffer, MAXRECL , inFile) != NULL) {
          fprintf(outFile, ">%s", lineBuffer);
        }
        fclose(inFile);
      }
    }
#ifdef _WIN32
    while (FindNextFile(hFind, &fdFile) != 0);
    FindClose(hFind);
#endif
  }
 
#ifdef __CMS
#else
#ifdef _WIN32
#else
  closedir(d);
#endif
#endif
 
  fclose(outFile);
 
  return 0;
}
 
char* validateFileName(char *listFileLine) {
  int i;
  /* TODO Check if the file is not a directory */
 
#ifdef __CMS
 
  int recl;
  char trimmedFileType[9];
 
  listFileLine[20] = 0;   /* 20 is the end of the file mode */
 
  /* If the file record length is too long skip */
  listFileLine[29] = 0;
  recl  = atoi(listFileLine+24);
  if (recl>MAXRECL) {
   return NULL;
  }
 
  /* position 9 is where the file type starts */
  for (i=9; listFileLine[i] && listFileLine[i]!=' '; i++) {
    trimmedFileType[i-9] = listFileLine[i];
  }
  trimmedFileType[i-9] = 0;
 
#else
 
  char buffer[100];
  strncpy(buffer, listFileLine, 100);
  buffer[99]=0;
  (void)strtok(buffer, ".");
  char *trimmedFileType = strtok(NULL, " .");
  if (trimmedFileType == NULL) {
    printf("WARNING: file %s skipped\n", listFileLine);
    return NULL;
  }
 
#endif
 
  /* Only process if the file is one of the included types */
  int noTypes = sizeof(includeTypes) / sizeof(includeTypes[0]);
 
  for (i=0; i<noTypes; ++i)
  {
    if (!strcmp(includeTypes[i], trimmedFileType)) {
      return listFileLine;
    }
  }
  printf("WARNING: file %s skipped\n", listFileLine);
  return NULL;
}
 
int split(void) {
  char* fileName;
  FILE* inFile;
  FILE* outFile = NULL;
  char lineBuffer[MAXRECL];
 
  inFile = fopen(CONCATFILE, "r");
  if (inFile==NULL) {
    printf("ERROR: Error opening file %s\n", CONCATFILE);
    return -1;
  }
  while (fgets(lineBuffer, MAXRECL , inFile) != NULL) {
    switch (lineBuffer[0]) {
      case '+':
        if (outFile) {
          fclose(outFile);
        }
        fileName = fromStoredName(lineBuffer+1);
        outFile = fopen(fileName, "w");
        if (outFile==NULL) {
          fclose(inFile);
          printf("ERROR: Error opening file %s\n", fileName);
          return -1;
        }
        break;
 
      case '>':
        if (!outFile) {
          printf("ERROR: Output file not specified on CONCAT file");
          fclose(inFile);
          return -1;
        }
#ifdef __CMS
        /* This is a work around for a bug in CMSSYS where it does
           not like writing a line with just a \n */
        if (strlen(lineBuffer)>2) {
          fputs(lineBuffer+1, outFile);
        }
        else {
          fputs(" \n", outFile);
        }
#else
        fputs(lineBuffer+1, outFile);
#endif
        break;
 
      default:
        if (outFile) {
          fclose(outFile);
        }
        fclose(inFile);
        printf("ERROR: Invalid character in col 0\n");
        return -1;
     }
  }
  if (outFile) {
    fclose(outFile);
  }
  fclose(inFile);
  return 0;
}
 
char* toStoredName(char *fileName) {
#ifdef __CMS
 static char name[18];
 int i;
 char *fn;
 char *ft;
 fn = strtok(fileName, " ");
 ft = strtok(NULL, " ");
 
 sprintf(name, "%s.%s",fn,ft);
 for (i=0; name[i]; i++) name[i] = tolower(name[i]);
 return name;
#else
 return basename(fileName);
#endif
}
 
char* fromStoredName(char *fileName) {
#ifdef __CMS
 static char name[18];
 int i;
 char *fn;
 char *ft;
 fn = strtok(fileName, ".");
 ft = strtok(NULL, " \n.");
 if (strlen(fn) > 8) fn[8]=0;
 if (strlen(ft) > 8) ft[8]=0;
 
 sprintf(name, "%s %s %s", fn, ft, drive);
 for (i=0; name[i]; i++) name[i] = toupper(name[i]);
 return name;
#else
 static char name[100];
 fileName[ strlen(fileName)-1 ]=0; /* Remove new-line */
 snprintf(name, 99, "%s/%s", drive, fileName);
 return name;
#endif
}
 
#ifdef __CMS
 
static const unsigned char lower[] = "abcdefghijklmnopqrstuvwxyz";
static const unsigned char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
 
int toupper(int c) {
  unsigned char *p;
  p=strchr(lower,c);
  if (p) return upper[p-lower];
  return c;
}
 
int tolower(int c) {
  unsigned char *p;
  p=strchr(upper,c);
  if (p) return lower[p-upper];
  return c;
}
 
#endif
