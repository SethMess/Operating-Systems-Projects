The executable files can be generated with the "make" command 

There are 4 executables generated from this code each with a different behavior
all executables take a disk image as the first parameter.
-disckinfo will print out information about the disk image passed as a parameter.
Example usage: ./diskinfo subdirs.img
-discklist will list out the contents of the root direcotory of the disk image or of a given subdirectory if one is given as a second parameter.
Example usage: ./diskinfo subdirs.img subdir1
-diskget will copy a file in the disk image to the local directory and takes the path of the file to copy as the second parameter and the name of what the copy should be called as the third parameter.
Example usage: ./diskget subdirs.img subdir1/subdir2/foo.txt output.txt 
-diskput is will not execute.

two disk images have been included to execute the code with.

Description of how the code is implemented:
I made all my code in one .c so I could reuse my helper functions in the different parts. I used the memory mapped file approach
to allow for easier access to directory entries and the superblock. In part 1 I cast the memory too a super block struct to get
all its information, then looped through the FAT to count the number of each type of entry. In part 2 I find the sub directory if
there is one and then loop through the entries of that directory or the root directory to print out all the files and other directories
In part 3 I found the file in the given directories or in the root directory then wrote the information in those blocks to a new file
I did not complete part 4 of this assignment, the executable will be generated but nothing will happen when it is run.

There are comments in the code explaining more detail.
