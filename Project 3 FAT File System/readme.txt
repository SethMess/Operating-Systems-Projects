The executable files can be generated with the "make" command 

I made all my code in one .c so I could reuse my helper functions in the different parts. I used the memory mapped file approach
to allow for easier access to directory entries and the superblock. In part 1 I cast the memory too a super block struct to get
all its information, then looped through the FAT to count the number of each type of entry. In part 2 I find the sub directory if
there is one and then loop through the entries of that directory or the root directory to print out all the files and other directories
In part 3 I found the file in the given directories or in the root directory then wrote the information in those blocks to a new file
I did not complete part 4 of this assignment, the executable will be generated but nothing will happen when it is run.

There are comments in the code explaining more detail.