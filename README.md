# Files11 -- PDP-11 File System Management

The Files11 utility is acommand line interface that provides some usefule commands to manage a files-11 disk file.

## Installing

## Supported commands

The supported commands are:

HELP
PWD
CD
DIR
CAT or TYPE
TIME
FREE
VFY
DEL or RM
DMPLBN
DMPHDR
IMPORT or UPLOAD
EXPORT or DOWNLOAD

### HELP

Print a list of available commands

### PWD

Print the current working directory

### CD

Change the current working directory

### DIR

List the content of the specified or current directory

### CAT or TYPE

Print the content of the dpecified file(s)

### TIME

Print the current time, in PDP-11 format.

### FREE

Print the current free disk space.

### VFY

Verify the integrity of the file system.
    /DV - To validate the directory structure and detects 'lost' files.

### DEL or RM

Delete the specified file(s)

### DMPLBN

Dump the specified logical block number.

### DMPHDR

Dump the specified file header, in PDP-11 format

### IMPORT or UPLOAD

Upload the specified file(s) in the specified directory or current working directory.

### EXPORT or DOWNLOAD

Download the specified file(s) from the specified directory or current working directory.
The individual files are saved on the host file system.

