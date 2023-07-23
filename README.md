# pfind-spfind

## Overview

There are two programs in this project. `pfind` is a program that recursively searches and returns the files matching the user provided permission string starting from a directory specified by the user. `spfind` is a program that uses pipes and fork/exec to sort the output of the `pfind` program and also add the total number of matches to the output.

## Functionality

### pfind

`pfind` is run from the command line and takes in two command flags along with their respective arguments. On success, `pfind` will print the files that match the given permission string starting in the given directory and return `EXIT_SUCCESS`. On failure, `pfind` returns `EXIT_FAILURE`. Usage:

     ./pfind -d <directory> -p <permissions string> [-h]

The directory from which to start the search should be specified after the -d flag, the permissions string to be specified after the -p flag, and the -h flag is an optional flag that shows the user how to execute the program. Incorrect usage will cause the program to exit in failure along with a descriptive error message.

The permissions string must be in proper format, with exactly nine characters that all must either be a dash or one of the characters 'rwx' in the proper position.

Some examples of valid permissions strings:

    rwxrwxrwx
    ---------
    rw-r--r--
    rwx------

Some examples of invalid permissions strings:

    abcdefghi
    xrwxrwxrw
    ---rrr---
    -rwxrwxrwxrwxrwxrwxrwxrwxrwx

If an invalid permission string is passed, the program exits in failure along with an appropriate error message.

If both a valid directory and permission string is passed in, the program recursively goes through the file system starting from the specified directory and prints out each file that matches the given permission string. The full path name of the files is printed.

### Sample Execution

For these examples, the directory tree will have the following format:


    test_dir
    ├── subdir1
    │   ├── file1
    │   └── file2
    └── subdir2
        ├── file1
        └── file2

    
The files in test_dir have the following permissions:

```
test_dir:
drwxrwxrwx subdir1
drwxr-xr-x subdir2

test_dir/subdir1:
---------- file1
-rw-r--r-- file2

test_dir/subdir2:
-rw-r--r-- file1
-rw-r--r-- file2
```

( ~ should expand to the full path of home directory )

```
    $ ./pfind -h
    Usage: ./pfind -d <directory> -p <permissions string> [-h]
    $ ./pfind -v -h
    Error: Unknown option '-v' received.
    $ ./pfind -d ~/test_dir
    Error: Required argument -p <permissions string> not found.
    $ ./pfind -p rwxrwxrwx
    Error: Required argument -d <directory> not found.
    $ ./pfind -d ~/test_dir -p badpermis
    Error: Permissions string 'badpermis' is invalid.
    $ ./pfind -d invalid_dir -p rwxrwxrwx
    Error: Cannot stat 'invalid_dir'. No such file or directory.
    $ ./pfind -d invalid_dir -p badpermis
    Error: Cannot stat 'invalid_dir'. No such file or directory.
    $ ./pfind -d ~/test_dir -p rwxrwxrwx -h
    Usage: pfind -d <directory> -p <permissions string> [-h]
    $ ./pfind -d ~/test_dir -p rw-r--r--
    ~/test_dir/subdir1/file2
    ~/test_dir/subdir2/file2
    ~/test_dir/subdir2/file1
    $ ./pfind -d ~/test_dir -p --x--x--x
    <no output>

Creating a new directory danger_dir with permissions `---------` running `pfind` on it should result in:

    $ mkdir -m 000 ~/danger_dir
    $ ./pfind -d ~/danger_dir -p --x--x--x
    Error: Cannot open directory '~/danger_dir'. Permission denied.
```
### spfind

`spfind` is run from the command line and also takes in the same command flags and arguments as `pfind`. Usage:

     ./spfind -d <directory> -p <permissions string> [-h]

`spfind` forks two children, one for `pfind` and one for `sort` (C standard library). Using pipes, the stdout of `pfind` is connected to the stdin of `sort`, and the stdout of `sort` is connected to the stdin of the parent process.

The parent process reads from the read-end of the `sort` pipe until it reaches end-of-file (`read()` returns 0), prints out all the text received, and reports how many lines were returned.

### Sample Execution

```
$ ./spfind
Usage: ./spfind -d <directory> -p <permissions string> [-h]

$ ./spfind -d ~ -p rwxr-xr-q
Error: Permissions string 'rwxr-xr-q' is invalid.
```

For the following output, we will operate under the following directory tree and
permissions.


    test_dir
    ├── subdir1
    │   ├── file1
    │   └── file2
    └── subdir2
    │   ├── file1
    │   └── file2
    └── danger

    
The files in test_dir have the following permissions:

```
test_dir:
drwxrwxrwx subdir1
drwxr-xr-x subdir2
---------- danger

test_dir/subdir1:
---------- file1
-rw-r--r-- file2

test_dir/subdir2:
-rw-r--r-- file1
-rw-r--r-- file2
```

```
$ ./spfind -d ~/test_dir -p rw-r--r--
Error: Cannot open directory '~/test_dir/danger'. Permission denied.
~/test_dir/subdir1/file2
~/test_dir/subdir2/file1
~/test_dir/subdir2/file2
Total matches: 3
```

Note the error message is written to standard error and is not part of the
total matches. However, the names of the files and number of total matches is
written to standard output.

