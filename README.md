# YATA - Yay! Another Text Archive
Lightweight tool to archive text files - designed to help move files between VM/370 mini disks and Linux/Windows.

# Usage
    Usage   :   yata [options]
    Options :
      -h        Prints help message
      -v        Prints Version
      -x        Extract from Archive
      -c        Create Archive
      -d TEXT   Drive or Directory of files (default: A in CMS, . in Windows/Linux)
      -f TEXT   Archive file (default: yata.txt)

# YATA Archive file format
The format is trivial. The YATA archive file is text based, variable length lines up to 80 characters (so it can go through punch card files). 

The key assumption is that ASCII/EBCDIC/Unicode conversions are handled by the transport layer not YATA.

The first character is a line control character
- File Start (+)

    +filename.filetype   

- Line Start (\>) first 79 characters

    \>linetext

- Line Continue (\<) next 79 characters etc.

    \<linetext

- Archive End (*)

    *

Trailing spaces are removed. On CMS flies will need to have recfm / lrecl reset (which I don't see as an issue)

# VMARC Archiving
This is the standard tool for VM/370 archiving, and it has GUI tools, etc. 
I suggest you use this for archives that you want to archive!
I am not attempting to replace VMARC but I wanted a really lightweight tool to transfer source directories, hence YATA.
