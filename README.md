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

Example: 
```
    DIR [*]         : List content of all directories.
    DIR [2,*]       : List content of all directories in group 2.
    DIR [2,54]      : List all files in directory [2,54].
    DIR [*]FILE.CMD : List all highest version instances of file 'FILE.CMD' in any directory.
    DIR FILE.CMD;*  : List all version(s) of 'FILE.CMD' on the current working directory.
```


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
Delete file(s) from a directory. The file specification can include the directory.
The file must specify a version number or '*' to delete all versions of the file(s).


### DMPLBN

```
>DMPLBN <lbn>
```
Dump the content of logical block number 'lbn'.
lbn must be a positive integer from 0 to number of blocks on the file system.
lbn is octal if prefixed with a '0'.
lbn is hexadecimal if prefixed with a 'x' or 'X' character.
otherwise lbn is decimal.


### DMPHDR

```
>DMPHDR <file-spec>
```

Dump the content of a file header (similar to PDP-11 'DMP /HD' command.)


### IMPORT or UPLOAD

```
>IMPORT <host-file-spec> [local-file-spec]
>IMP    <host-file-spec> [local-file-spec]
>UP     <host-file-spec> [local-file-spec]
```
This command import/upload files from the host file system to the PDP-11 disk.
The host file specifications can include wildcard and uses '/' directory delimiter.
The local file specifier can specify a destination directory or default to the current directory.
A specific local file can be specified if uploading a single file (useful if the host file doesn't
match the 9.3 naming restrictions.)

Example: 
```
>IMP data/*.txt [100,200]
```
Will upload all .txt files from the host data directory to the [100,200] directory.
```
>IMPORT longfilename.txt [100,200]LONG.TXT
```
Will upload all .txt files from the host data directory to the [100,200] directory.


### EXPORT or DOWNLOAD

```
>EXPORT <local-file-spec> [host-directory]";
>EXP    <local-file-spec> [host-directory]";
>DOWN   <local-file-spec> [host-directory]\n";
```
Export or download PDP-11 files to the host file system.
Example: 
```
    >EXP [*]
```
Export the whole PDP-11 volume to the host current working directory under a sub-directory named <volume-name>.

```
    >EXP [100,200]
```
Export the content of the [100,200] directory to the host current working directory under a sub-directory named '<volume-name>/100200'.

```
>EXP [3,54]*.CMD
```
Export the latest version of all .CMD files under the [3,54] directory to the host current working directory under a sub-directory named '<volume-name>/003054'.

