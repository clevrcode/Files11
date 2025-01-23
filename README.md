# Files11 -- PDP-11 File System Management

The Files11 utility is a command line interface that provides some useful commands to manage a files-11 disk file.

The code is based on the [DEC Files-11 Specification document from April 1981.](http://www.bitsavers.org/pdf/dec/pdp11/rsx11m_s/Files-11_ODS-1_Spec_Apr81.pdf). All reference in the source code refer to this document.


## Installing



## Supported commands

The supported commands are:

* HELP
* PWD
* CD
* DIR
* CAT or TYPE
* DEL or RM
* FREE
* VFY
* DMPLBN
* DMPHDR
* IMPORT or UPLOAD
* EXPORT or DOWNLOAD

### HELP

The HELP command displays a brief description of a command usage.

For more details on a specific command, enter

```
>HELP <command>
```

### PWD

```
>PWD
```

Displays the current working directory.

### CD

```
>CD [nnn, mmm]
>CD [dirname]
```

Change the current working directory

### DIR

```
>DIR <UFD>[file-spec]
```

List the content of the specified directory/directories
The directory and file specification can include '*' wildcard character.

Example: DIR [*]         : List content of all directories.";
         DIR [2,*]       : List content of all directories in group 2.";
         DIR [2,54]      : List all files in directory [2,54].";
         DIR [*]FILE.CMD : List all highest version instances of file 'FILE.CMD' in any directory.";
         DIR FILE.CMD;*  : List all version(s) of 'FILE.CMD' on the current working directory.";

### CAT or TYPE

```
>CAT <file-spec>
>TYPE <file-spec>
```
Display the content of a file.
If the file contains binary, a binary dump will be displayed instead.

### FREE

```
>FREE
```

Display the amount of available space on the disk, the largest contiguous space,
the number of available headers and the number of headers used.
This command is similar to PDP-11 'PIP /FR' command.

### VFY

Verify the integrity of the file system.
    /DV - To validate the directory structure and detects 'lost' files.

### DEL or RM

```
>RM [<UFD>]<file-spec>
>DEL [<UFD>]<file-spec>
```

Delete file(s) from a directory. The file specification can have a directory.
The file must specify a version number or '*' to delete all versions of the file(s).

### DMPLBN

Dump the specified logical block number.

### DMPHDR

Dump the specified file header, in PDP-11 format

### IMPORT or UPLOAD

Upload the specified file(s) in the specified directory or current working directory.

### EXPORT or DOWNLOAD

Download the specified file(s) from the specified directory or current working directory.
The individual files are saved on the host file system.

