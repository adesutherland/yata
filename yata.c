/***********************************************************************/
/*                                                                     */
/* YATA.C                                                              */
/* Yay! Another Text Archive. Lightweight tool to archive text files   */
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
#define ARCLINELEN 80
#define VERSION "1.2.0"

#ifdef __CMS

#include <cmssys.h>
static char* includeTypes[] = { "C", "H", "EXEC", "ASSEMBLE", "LISTING", 
              "COPY", "MACLIB",
             "MACRO", "PARM", "MEMO", "HELPCMD", "HELPCMD2" };
#define ARCHIVE "YATA TXT A1"
#define DRIVE "A"
static int toupper(int c);
static int tolower(int c);

#define FILENAMELEN 25
static char fileNameBuffer[FILENAMELEN];

#else

#ifdef _WIN32

#include <windows.h>

#else

#include <dirent.h>
#include <libgen.h>

#endif

#include <ctype.h>
static char* includeTypes[] = { "c", "h", "exec", "assemble", "listing",
              "copy", "maclib",
             "macro", "parm", "memo", "helpcmd", "helpcmd2" };
#define ARCHIVE "yata.txt"
#define DRIVE "."

#endif

static char* toUpperString(char* string);
static int create_archive();
static int extract_archive();
static char* validateFileName(char* listFileLine);
static char* getFileName(char* listFileLine);
static char* toStoredName(char* fileName);
static char* fromStoredName(char* fileName);
static char* trimTrailingSpace(char* str);
static char* trimTrailingNL(char* str);


#ifdef _WIN32
static char* basename(char* path)
{
  char* base = strrchr(path, '\\');
  return base ? base + 1 : path;
}
#endif

static char* drive;
static char* archive_file;

static void help() {
  char* helpMessage =
    "YATA (Yay! Another Text Archive) - "
    "Lightweight tool to archive text files\n"
    "Version :   " VERSION "\n"
    "Usage   :   yata [options]\n"
    "Options :\n"
    "  -h        Prints help message\n"
    "  -v        Prints Version\n"
    "  -x        Extract from Archive\n"
    "  -c        Create Archive\n"
    "  -d TEXT   Drive or Directory of files (default: A in CMS,"
    " . in Windows/Linux)\n"
    "  -f TEXT   Archive file (default: yata.txt)\n";

#ifdef __CMS
  helpMessage = toUpperString(helpMessage);
#endif        
  printf(helpMessage);
}

static void error_and_exit(int rc, char* message) {

#ifdef __CMS
  message = toUpperString(message);
#endif        

  fprintf(stderr, "ERROR: %s\n", message);
  help();
  exit(rc);
}

int main(int argc, char* argv[]) {
  int extract_mode = -1;
  int i;
  drive = DRIVE;
  archive_file = ARCHIVE;

  /* Parse arguments - DIY style! */
  for (i = 1; i < argc && argv[i][0] == '-'; i++) {
    if (strlen(argv[i]) > 2) {
      error_and_exit(2, "Invalid argument");
    }
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
#ifdef __CMS
      fileNameBuffer[0] = 0;
      for (i++; i < argc && argv[i][0] != '-'; i++) {
        strncat(fileNameBuffer, " ", FILENAMELEN - strlen(fileNameBuffer)
          - 1);
        strncat(fileNameBuffer, argv[i], FILENAMELEN - strlen(fileNameBuffer)
          - 1);
      }
      i--;
      if (strlen(fileNameBuffer) == 0) {
        error_and_exit(2, "Missing filename after -f");
      }
      archive_file = fileNameBuffer + 1;
#else
      i++;
      if (i >= argc) {
        error_and_exit(2, "Missing filename after -f");
      }
      archive_file = argv[i];
#endif
      break;

    case 'V': /* Version */
      printf("%s\n", VERSION);
      exit(0);

    case 'H': /* Help */
      help();
      exit(0);

    default:
      error_and_exit(2, "Invalid argument");
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

static int create_archive() {
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

  outFile = fopen(archive_file, "w");

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
          return 1;
        }
        fprintf(outFile, "+%s\n", toStoredName(fileName));
        while (fgets(lineBuffer, MAXRECL, inFile) != NULL) {
          trimTrailingSpace(lineBuffer);
          char* line = lineBuffer;
          if (strlen(line) > ARCLINELEN - 1) {
            fprintf(outFile, ">%.*s\n", ARCLINELEN - 1, line);
            line += 79;
            do {
              if (strlen(line) > 79) {
                fprintf(outFile, "<%.*s\n", ARCLINELEN - 1, line);
                line += 79;
              }
              else {
                fprintf(outFile, "<%s\n", line);
                break;
              }
            } while (1);
          }
          else fprintf(outFile, ">%s\n", line);
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
  fprintf(outFile, "*\n");
  fclose(outFile);

  return 0;
}

static char* validateFileName(char* listFileLine) {
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

static void writeLine(FILE* outFile, char* lineBuffer) {
  trimTrailingSpace(lineBuffer);

#ifdef __CMS
  /* This is a work around for a bug in CMSSYS where it does
     not like writing a line with just a \n */
  if (strlen(lineBuffer) > 0) {
    fprintf(outFile, "%s\n", lineBuffer);
  }
  else {
    fputs(" \n", outFile); /* Have to add a space :-( */
  }
#else
  fprintf(outFile, "%s\n", lineBuffer);
#endif
}

static int extract_archive() {
  int lineNo = 0;
  char* fileName;
  FILE* inFile;
  FILE* outFile = NULL;
  char lineBuffer[MAXRECL + 1];
  char line[ARCLINELEN + 3]; /* could add /r /n /0 */
  int needToWriteLine = 0;
  int startedArchive = 0;

  inFile = fopen(archive_file, "r");
  if (inFile == NULL) {
    printf("ERROR: Error opening archive file %s\n", archive_file);
    return 1;
  }
  while (fgets(line, ARCLINELEN + 3, inFile) != NULL) {
    trimTrailingNL(line);
    lineNo++;
    if (strlen(line) > 80) {
      if (startedArchive) {
        if (outFile) {
          if (needToWriteLine)
            writeLine(outFile, lineBuffer);
          fclose(outFile);
        }
        fclose(inFile);
        printf("ERROR: Line %d, Line >80 chars \"%s\"\n", lineNo, line);
        return 1;
      }
    }

    switch (line[0]) {
    case '*':
      startedArchive = 1;
      break;

    case '+':
      startedArchive = 1;
      if (outFile) {
        if (needToWriteLine) {
          if (strlen(lineBuffer) > 0) {
            writeLine(outFile, lineBuffer);
          }
          needToWriteLine = 0;
          lineBuffer[0] = 0;
        }
        fclose(outFile);
      }
      fileName = fromStoredName(line + 1);
      outFile = fopen(fileName, "w");
      if (outFile == NULL) {
        fclose(inFile);
        printf("ERROR: Error opening file %s\n", fileName);
        return 1;
      }
      break;

    case '>':
      startedArchive = 1;
      if (!outFile) {
        printf("ERROR: Output file not specified, error in archive file");
        fclose(inFile);
        return 1;
      }
      if (needToWriteLine) {
        writeLine(outFile, lineBuffer);
      }
      strncpy(lineBuffer, line + 1, MAXRECL - 1);
      needToWriteLine = 1;
      break;

    case '<':
      startedArchive = 1;
      if (!needToWriteLine) {
        printf("ERROR: Append line without a line, error in archive file");
        fclose(inFile);
        return 1;
      }
      strncat(lineBuffer, line + 1, MAXRECL - strlen(lineBuffer) - 1);
      break;

    default:
      /* Skip any junk before the start */
      if (startedArchive) {
        if (outFile) {
          if (needToWriteLine)
            writeLine(outFile, lineBuffer);
          fclose(outFile);
        }
        fclose(inFile);
        printf("ERROR: Line %d, Invalid character in col 0 of \"%s\"\n",
          lineNo, line);
        return 1;
      }
    }
  }

  if (outFile) {
    if (needToWriteLine) {
      if (strlen(lineBuffer) > 0) {
        writeLine(outFile, lineBuffer);
      }
    }
    fclose(outFile);
  }
  fclose(inFile);
  return 0;
}

static char* toStoredName(char* fileName) {
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

static char* fromStoredName(char* fileName) {
#ifdef __CMS
  static char name[25];
  int i;
  char* fn = fileName;
  char* ft;

  for (ft = fn; *ft; ft++) if (*ft == '.') {
    *ft = 0; ft++;
    break;
  }

  if (strlen(fn) > 8) fn[8] = 0;
  if (strlen(ft) > 8) ft[8] = 0;

  sprintf(name, "%s %s %s", fn, ft, drive);
  for (i = 0; name[i]; i++) name[i] = toupper(name[i]);
  return name;
#else
  static char name[100];
  snprintf(name, 99, "%s/%s", drive, fileName);
  return name;
#endif
}


static char* trimTrailingSpace(char* str)
{
  char* end;

  end = str + strlen(str) - 1;
  while (end >= str &&
    (*end == ' ' || *end == '\n' || *end == '\t' || *end == '\r')
    ) end--;

  /* Terminate */
  end[1] = 0;

  return str;
}

static char* trimTrailingNL(char* str)
{
  char* end;

  end = str + strlen(str) - 1;
  while (end >= str && (*end == '\n' || *end == '\r'))
    end--;

  /* Terminate */
  end[1] = 0;

  return str;
}

static char* toUpperString(char* string) {
  int i;
  for (i = 0; string[i]; i++) string[i] = toupper(string[i]);
  return string;
}

#ifdef __CMS

static const unsigned char lower[] = "abcdefghijklmnopqrstuvwxyz";
static const unsigned char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int toupper(int c) {
  unsigned char* p;
  p = strchr(lower, c);
  if (p) return upper[p - lower];
  return c;
}

static int tolower(int c) {
  unsigned char* p;
  p = strchr(upper, c);
  if (p) return lower[p - upper];
  return c;
}

#endif
