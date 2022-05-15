## Compilation

```
g++ main.cpp
```
## Run

```
./a.out
```

## Disk Creation and mounting
### 1: create disk
- Enter the name of the disk
- creates a disk of 500 MB
### 2: mount disk
- Enter the name of the disk
- Reads the file's and disk's metadata back from disk
### 3: exit
- Exits the program

## File Operations

### 1: create file
- Enter the name of the file
### 2: open file
- Enter the name of the file and mode in which to open

    0 - Read mode

    1 - Write mode

    2 -  Append mode
- Returns a file descriptor
### 3: read file
- Enter the file descriptor returned by __Open file__
- Make sure you opened the file in Read mode
### 4: write file
- Enter the file descriptor returned by __Open file__
- Make sure you opened the file in Write mode
- Enter the contents of the file
### 5: append file
- Enter the file descriptor returned by __Open file__
- Make sure you opened the file in Append mode
- Enter the contents of the file
### 6: close file
- Enter the file descriptor returned by __Open file__
### 7: delete file
- Enter the file name
- Make sure you have closed the file
### 8: list of files
- Return the list of files
### 9: list of opened files
- Return the list of opened files
### 10: unmount
- Writes the file's and disk's metadata back to disk
