/***********************************************************************/
/*                                                                     */
/* YATA.C                                                              */
/* Yet Another Text Archive. Lightweight tool to archive text files    */
/*                                                                     */
/* Adrian Sutherland                                                   */
/*                                                                     */
/***********************************************************************/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXRECL 800
#define VERSION "F0001"

#ifdef __CMS

#include <cmssys.h>
char* includeTypes[] = { "C", "H", "EXEC", "ASSEMBLE", "LISTING", "COPY", "MACLIB",
             "MACRO", "PARM", "MEMO" };
#define ARCHIVE "YATA TXT A1"
#define DRIVE "A"
int toupper(int c);
int tolower(int c);

#else

#ifdef _WIN32

#include <windows.h>

#else

#include <dirent.h>
#include <libgen.h>

#endif

#include <ctype.h>
char* includeTypes[] = { "c", "h", "exec", "assemble", "listing", "copy", "maclib",
             "macro", "parm", "memo" };
#define ARCHIVE "yata.txt"
#define DRIVE "."

#endif

char* toUpperString(char* string);
int create_archive();
int extract_archive();
char* validateFileName(char* listFileLine);
char* getFileName(char* listFileLine);
char* toStoredName(char* fileName);
char* fromStoredName(char* fileName);
char* trimTrailingSpace(char* str);

#ifdef _WIN32
char* basename(char* path)
{
  char* base = strrchr(path, '\\');
  return base ? base + 1 : path;
}
#endif

char* drive;
char* archive_file;

void help() {
  char* helpMessage =
    "yata (Yet Another Text Archive); Lightweight tool to archive text files\n"
    "Version :   " VERSION "\n"
    "Usage   :   yata [options]\n"
    "Options :\n"
    "  -h        Prints help message\n"
    "  -v        Prints Version\n"
    "  -x        Extract from Archive\n"
    "  -c        Create Archive\n"
    "  -d TEXT   Drive or Directory of files (default: A in CMS, . in Windows/Linux)\n"
    "  -f TEXT   Archive file (default: yata.txt)\n";

#ifdef __CMS
  helpMessage = toUpperString(helpMessage);
#endif        
  printf(helpMessage);
}

void error_and_exit(int rc, const char* message) {

#ifdef __CMS
  message = toUpperString(message);
#endif        

  fprintf(stderr, "ERROR: %s\n", message);
  help();
  exit(rc);
}

int main(int argc, char* argv[]) {
  int extract_mode = -1;
  drive = DRIVE;
  archive_file = ARCHIVE;
  int i;

  /* Parse arguments - DIY style! */
  for (i = 1; i < argc && argv[i][0] == '-'; i++) {
    switch (toupper((argv[i][1]))) {
    case '-':
      break;

    case 'X': /* Extract*/
      extract_mode = 1;
      break;

    case 'C': /* Create */
      extract_mode = 0;
      break;

    case 'D': /* Directory / Drive */
      i++;
      if (i >= argc) {
        error_and_exit(2, "Missing drive/directory after -d");
      }
      drive = argv[i];
      break;

    case 'F': /* Archive file */
      i++;
      if (i >= argc) {
        error_and_exit(2, "Missing filename after -f");
      }
      archive_file = argv[i];
      break;

    case 'V': /* Version */
      printf("%s\n", VERSION);
      exit(0);

    case 'H': /* Help */
      help();
      exit(0);

    default:
      error_and_exit(2, "Invalid Arguments");
    }
  }

  if (i < argc) {
    error_and_exit(2, "Unexpected Arguments");
  }

  if (extract_mode == -1) {
    error_and_exit(2, "Create or eXtract must be specified");
  }

  if (extract_mode) {
    return extract_archive();
  }
  return create_archive();
}

int create_archive() {
#ifdef __CMS
  int stackSize;
  int files;
  char consoleLine[131];
#else
#ifdef _WIN32
  WIN32_FIND_DATA fdFile;
  HANDLE hFind = NULL;
#else
  DIR* d;
  struct dirent* dir;
#endif
#endif
  char* fileName;
  FILE* inFile;
  FILE* outFile;
  char lineBuffer[MAXRECL];

  outFile = fopen(ARCHIVE, "w");

#ifdef __CMS
  int i;
  char command[40];
  stackSize = CMSstackQuery();
  sprintf(command, "LISTFILE * * %s (FORMAT STACK", drive);
  CMScommand(command, CMS_COMMAND);
  files = CMSstackQuery() - stackSize;
  for (i = 0; i < files; i++) {
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
  if (d) {
    char dirAndName[260];
    while ((dir = readdir(d)) != NULL) {
      snprintf(dirAndName, 259, "%s/%s", drive, dir->d_name);
      fileName = validateFileName(dirAndName);

#endif
#endif

      if (fileName) {
        inFile = fopen(fileName, "r");
        if (inFile == NULL) {
          printf("ERROR: Can not open file %s\n", fileName);
          return -1;
        }
        fprintf(outFile, "+%s\n", toStoredName(fileName));

        while (fgets(lineBuffer, MAXRECL, inFile) != NULL) {
          trimTrailingSpace(lineBuffer);
          fprintf(outFile, ">%s\n", lineBuffer);
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

char* validateFileName(char* listFileLine) {
  int i;
  /* TODO Check if the file is not a directory */

#ifdef __CMS

  int recl;
  char trimmedFileType[9];

  listFileLine[20] = 0;   /* 20 is the end of the file mode */

  /* If the file record length is too long skip */
  listFileLine[29] = 0;
  recl = atoi(listFileLine + 24);
  if (recl > MAXRECL) {
    return NULL;
  }

  /* position 9 is where the file type starts */
  for (i = 9; listFileLine[i] && listFileLine[i] != ' '; i++) {
    trimmedFileType[i - 9] = listFileLine[i];
  }
  trimmedFileType[i - 9] = 0;

#else

  char buffer[100];
  strncpy(buffer, listFileLine, 100);
  buffer[99] = 0;
  (void)strtok(buffer, ".");
  char* trimmedFileType = strtok(NULL, " .");
  if (trimmedFileType == NULL) {
    printf("WARNING: file %s skipped\n", listFileLine);
    return NULL;
  }

#endif

  /* Only process if the file is one of the included types */
  int noTypes = sizeof(includeTypes) / sizeof(includeTypes[0]);

  for (i = 0; i < noTypes; ++i)
  {
    if (!strcmp(includeTypes[i], trimmedFileType)) {
      return listFileLine;
    }
  }
  printf("WARNING: file %s skipped\n", listFileLine);
  return NULL;
}

int extract_archive() {
  char* fileName;
  FILE* inFile;
  FILE* outFile = NULL;
  char lineBuffer[MAXRECL];

  inFile = fopen(ARCHIVE, "r");
  if (inFile == NULL) {
    printf("ERROR: Error opening file %s\n", ARCHIVE);
    return -1;
  }
  while (fgets(lineBuffer, MAXRECL, inFile) != NULL) {
    switch (lineBuffer[0]) {
    case '+':
      if (outFile) {
        fclose(outFile);
      }
      fileName = fromStoredName(lineBuffer + 1);
      outFile = fopen(fileName, "w");
      if (outFile == NULL) {
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
      trimTrailingSpace(lineBuffer);
#ifdef __CMS
      /* This is a work around for a bug in CMSSYS where it does
         not like writing a line with just a \n */
      if (strlen(lineBuffer) > 1) {
        fprintf(outFile, "%s\n", lineBuffer + 1);
      }
      else {
        fputs(" \n", outFile); /* Have to add a space :-( */
      }
#else
      fprintf(outFile, "%s\n", lineBuffer + 1);
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

char* toStoredName(char* fileName) {
#ifdef __CMS
  static char name[25];
  int i;
  char* fn;
  char* ft;
  fn = strtok(fileName, " ");
  ft = strtok(NULL, " ");

  sprintf(name, "%s.%s", fn, ft);
  for (i = 0; name[i]; i++) name[i] = tolower(name[i]);
  return name;
#else
  return basename(fileName);
#endif
}

char* fromStoredName(char* fileName) {
#ifdef __CMS
  static char name[25];
  int i;
  char* fn = fileName;
  char* ft;

  for (ft = fn; *ft; ft++) if (*ft == '.') {
    *ft = 0; ft++;
    break;
  }
  TrimTrailingSpace(ft);

  if (strlen(fn) > 8) fn[8] = 0;
  if (strlen(ft) > 8) ft[8] = 0;

  sprintf(name, "%s %s %s", fn, ft, drive);
  for (i = 0; name[i]; i++) name[i] = toupper(name[i]);
  return name;
#else
  static char name[100];
  fileName[strlen(fileName) - 1] = 0; /* Remove new-line */
  snprintf(name, 99, "%s/%s", drive, fileName);
  return name;
#endif
}


char* trimTrailingSpace(char* str)
{
  char* end;

  end = str + strlen(str) - 1;
  while (end >= str &&
    (*end == ' ' || *end == '\n' || *end == '\t')
    ) end--;

  /* Terminate */
  end[1] = 0;

  return str;
}

char* toUpperString(char* string) {
  for (int i = 0; string[i]; i++) string[i] = toupper(string[i]);
  return string;
}

#ifdef __CMS

static const unsigned char lower[] = "abcdefghijklmnopqrstuvwxyz";
static const unsigned char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int toupper(int c) {
  unsigned char* p;
  p = strchr(lower, c);
  if (p) return upper[p - lower];
  return c;
}

int tolower(int c) {
  unsigned char* p;
  p = strchr(upper, c);
  if (p) return lower[p - upper];
  return c;
}

#endif
