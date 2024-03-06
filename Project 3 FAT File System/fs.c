#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

// Constants
#define FREE 0x00000000
#define RESERVED 0x00000001
#define LAST 0xFFFFFFFF
#define FAT_ENTRY_SIZE 4

// Super block
struct __attribute__((__packed__)) superblock_t
{
    uint8_t fs_id[8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

// Time and date entry
struct __attribute__((__packed__)) dir_entry_timedate_t
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Directory entry
struct __attribute__((__packed__)) dir_entry_t
{
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t create_time;
    struct dir_entry_timedate_t modify_time;
    uint8_t filename[31];
    uint8_t unused[6];
};

// This function opens and returns the File pointer while also handling any errors
FILE *open_file(char *name)
{
    FILE *file = fopen(name, "r");
    if (file == NULL)
    {
        printf("ERROR: could not open file\n");
        exit(1);
    }
    return file;
}

// This function maps the file onto memory and returns a pointer to it
unsigned char *get_memory_map(FILE *file, off_t *file_size)
{
    int file_num = fileno(file);
    struct stat fileStat;
    if (fstat(file_num, &fileStat) == -1)
    {
        printf("ERROR: could not get file status\n");
        fclose(file);
        exit(1);
    }

    off_t fileSize = fileStat.st_size;
    *file_size = fileSize;
    unsigned char *file_memory = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, file_num, 0);
    return file_memory;
}

// This funtion returns the next block of a file or directory from a fat
uint32_t get_next_block(unsigned char *base_file_memory, struct superblock_t *superblock, uint32_t entry_num)
{
    size_t start_of_fat = htonl(superblock->fat_start_block) * htons(superblock->block_size);
    unsigned char *fat_pointer = base_file_memory + start_of_fat; // moves to start of FAT
    fat_pointer += (uint32_t)(entry_num * sizeof(uint32_t));      // moves to entry of the fat correlating to the next entry
    return htonl(*((uint32_t *)fat_pointer));
}

// This function prints out a given file or directory entry based on the given format
void print_dir_entry(struct dir_entry_t *entry)
{
    struct dir_entry_timedate_t time = entry->create_time;
    if (entry->status == 3) // file
    {
        printf("F %10u %30s %u/%.2u/%.2u %u:%.2u:%.2u\n", htonl(entry->size), entry->filename, htons(time.year), time.month, time.day, time.hour, time.minute, time.second);
    }
    else if (entry->status == 5) // directory
    {
        printf("D %10u %30s %u/%.2u/%.2u %u:%.2u:%.2u\n", htonl(entry->size), entry->filename, htons(time.year), time.month, time.day, time.hour, time.minute, time.second);
    }
}

// returns the pointer to the memory of an entry given a specific file or directory name
size_t find_file_in_dir(unsigned char *base_file_memory, struct superblock_t *superblock, uint32_t start_block, char *name, int file_type)
{
    if (file_type != 3 && file_type != 5)
    {
        printf("ERROR: incorrect file type given");
        exit(1);
    }
    size_t block_size = htons(superblock->block_size);
    size_t entries_per_block = block_size / sizeof(struct dir_entry_t);
    uint32_t cur_block = start_block;
    struct dir_entry_t *entry = (struct dir_entry_t *)(base_file_memory + (block_size * cur_block));
    // loop while the block in the FAT is not 0xFFFFFFFF meaning the directory ended
    while (cur_block != LAST)
    {
        // loops through each entry in this block of the directory
        entry = (struct dir_entry_t *)(base_file_memory + (cur_block * block_size));
        for (size_t i = 0; i < entries_per_block; i++)
        {
            // returns the offset if the value is correct
            if (entry->status == file_type && strcmp((char *)(entry->filename), name) == 0)
            {
                return (size_t)((unsigned char *)entry - base_file_memory);
            }

            entry++;
        }
        cur_block = get_next_block(base_file_memory, superblock, cur_block);
    }
    // prints an error and returns if the file or directory is not found
    printf("ERROR: File '%s' not found.\n", name);
    exit(1);
    return -1;
}

// This function returns the block number of the sub given sub directory
uint32_t goto_sub_dir(unsigned char *base_file_memory, struct superblock_t *superblock, char *subdir_path)
{
    // sets up variables
    size_t block_size = htons(superblock->block_size);
    size_t start_of_root = (htonl(superblock->root_dir_start_block)) * block_size;
    size_t entries_per_block = block_size / sizeof(struct dir_entry_t);

    uint32_t cur_block = htonl(superblock->root_dir_start_block);

    // Parses the strings of the directory path given
    char path[strlen(subdir_path) + 1];
    strcpy(path, subdir_path);

    char *token = strtok(path, "/");
    int next_block_count = 0;
    struct dir_entry_t *entry = (struct dir_entry_t *)(base_file_memory + start_of_root);
    // loops through each directory in the given path
    while (token != NULL)
    {
        next_block_count = 0;

        // looping through all the directory blocks directory
        while (cur_block != LAST)
        {
            if (entry->status == 5 && strcmp((char *)(entry->filename), token) == 0)
            {
                // Move to the next subdirectory
                cur_block = htonl(entry->starting_block);
                entry = (struct dir_entry_t *)(base_file_memory + (cur_block * block_size));
                break; // Exit the inner loop once the subdirectory is found
            }
            else if (next_block_count == entries_per_block)
            {
                // moves to the next block after visiting all the directory entries
                cur_block = get_next_block(base_file_memory, superblock, cur_block);
                entry = (struct dir_entry_t *)(base_file_memory + (cur_block * block_size));
                next_block_count = 0;
            }
            else
            {
                entry++;
                next_block_count++;
            }
        }
        // if the function reaches the end of the last block of the file the sub directory has not been found and the program exits
        if (cur_block == LAST)
        {
            printf("ERROR: Subdirectory '%s' not found.\n", token);
            exit(1);
        }
        token = strtok(NULL, "/");
    }
    // return the block of the start of the next directory
    return cur_block;
}

void walk_the_fat(FILE *file)
{
}

// part 1
void diskinfo(int argc, char *argv[])
{
    // check command line arguments
    if (argc != 2)
    {
        printf("ERROR: Incorrect command line arguments\n");
        exit(1);
    }

    // get the file and memory map
    FILE *file = open_file(argv[1]);
    off_t file_size;
    unsigned char *file_memory = get_memory_map(file, &file_size);
    struct superblock_t *superblock = (struct superblock_t *)file_memory;

    // print superblock information
    printf("Super block information\n");
    printf("Block size: %u\n", htons(superblock->block_size));
    printf("Block count: %u\n", htonl(superblock->file_system_block_count));
    printf("FAT starts: %u\n", htonl(superblock->fat_start_block));
    printf("FAT blocks: %u\n", htonl(superblock->fat_block_count));
    printf("Root directory starts: %u\n", htonl(superblock->root_dir_start_block));
    printf("Root directory blocks: %u\n", htonl(superblock->root_dir_block_count));

    size_t block_size = htons(superblock->block_size);
    size_t num_fat_blocks = htonl(superblock->fat_block_count);
    size_t fat_start_block = htonl(superblock->fat_start_block);

    int block_count = 0;
    int free_count = 0;
    int reserved_count = 0;
    int allocated_count = 0;

    size_t fatSize = block_size * num_fat_blocks; // size of FAT in bytes
    size_t numEntries = fatSize / FAT_ENTRY_SIZE;
    size_t fat_offset = (fat_start_block) * (block_size);
    uint32_t *fat_entry = (uint32_t *)(file_memory + fat_offset);
    uint32_t cur_entry;

    // calculate FAT information
    while (block_count < numEntries)
    {
        cur_entry = htonl(*fat_entry);

        if (cur_entry == FREE)
        {
            free_count++;
        }
        else if (cur_entry == RESERVED)
        {
            reserved_count++;
        }
        else
        {
            allocated_count++;
        }
        block_count++;
        fat_entry++;
    }

    // Print FAT informations
    printf("FAT information\n");
    printf("Free blocks: %d\n", free_count);
    printf("Reserved blocks: %d\n", reserved_count);
    printf("Allocated blocks: %d\n", allocated_count);

    // unmap memory and close file
    munmap(superblock, file_size);
    fclose(file);
}

// part 2
void disklist(int argc, char *argv[])
{
    // check command line arguments
    if (argc < 2 || argc > 3)
    {
        printf("ERROR: Incorrect command line arguments\n");
        exit(1);
    }

    // get the file and memory map
    FILE *file = open_file(argv[1]);
    off_t file_size;
    unsigned char *file_memory = get_memory_map(file, &file_size);

    // get superblock information
    struct superblock_t *superblock = (struct superblock_t *)file_memory;
    size_t start_of_root = htonl(superblock->root_dir_start_block);
    size_t block_size = htons(superblock->block_size);
    // size_t start_of_root_in_bytes = (start_of_root)*block_size;

    size_t entries_per_block = block_size / sizeof(struct dir_entry_t);

    struct dir_entry_t *entry;
    uint32_t cur_block = start_of_root;

    // sets the directory block
    if (argc == 3)
    {
        cur_block = goto_sub_dir(file_memory, superblock, argv[2]);
    }

    // loops through each block of the directory and prints out each other directory and file
    while (cur_block != LAST)
    {
        entry = (struct dir_entry_t *)(file_memory + (cur_block * block_size));
        for (size_t i = 0; i < entries_per_block; i++)
        {
            print_dir_entry(entry);
            entry++;
        }
        cur_block = get_next_block(file_memory, superblock, cur_block);
    }
    // unmaps memory and closes file
    munmap(file_memory, file_size);
    fclose(file);
}

// part 3
void diskget(int argc, char *argv[])
{
    // check command line arguments
    if (argc != 4)
    {
        printf("ERROR: Incorrect command line arguments\n");
        exit(1);
    }

    // get the file and memory map
    FILE *file = open_file(argv[1]);
    off_t file_size;
    unsigned char *file_memory = get_memory_map(file, &file_size);

    struct superblock_t *superblock = (struct superblock_t *)file_memory;
    size_t start_block = htonl(superblock->root_dir_start_block);
    size_t block_size = htons(superblock->block_size);

    // locate the File
    char file_path[strlen(argv[2]) + 1];
    strcpy(file_path, argv[2]);

    // separate directory path and file name
    char *file_name = strrchr(file_path, '/');
    char *directory = NULL;

    if (file_name != NULL)
    {
        *file_name = '\0';     // Null-terminate to separate directory and file name
        file_name++;           // Move to the character after '/' so it is just the file name
        directory = file_path; // Set directory to the beginning of the string
    }
    else
    {
        file_name = file_path; // if path doesnt have a '/' file name is just the name
    }

    if (directory != NULL)
    {
        start_block = goto_sub_dir(file_memory, superblock, directory);
    }

    size_t offset = find_file_in_dir(file_memory, superblock, start_block, file_name, 3);

    struct dir_entry_t *entry = (struct dir_entry_t *)(file_memory + offset);
    uint32_t output_file_size = entry->size;
    uint32_t cur_file_block = htonl(entry->starting_block);

    // opens a file to write to
    FILE *write_file = fopen(argv[3], "wb");

    if (write_file == NULL)
    {
        printf("ERROR: could not open file\n");
        munmap(file_memory, file_size);
        fclose(file);
        exit(1);
    }
    uint32_t bytes_written = 0;
    uint32_t bytes_to_write = block_size;
    // loops through each block of the file and writes its contents to the file
    while (cur_file_block != LAST)
    {
        size_t writing_offset = cur_file_block * block_size;
        if ((output_file_size - bytes_written) < block_size)
        {
            bytes_to_write = output_file_size - bytes_written;
        }
        else
        {
            bytes_to_write = block_size;
        }
        fwrite(file_memory + writing_offset, 1, bytes_to_write, write_file);
        bytes_written += bytes_to_write;
        cur_file_block = get_next_block(file_memory, superblock, cur_file_block);
    }
    // unmaps the memory and closes the files
    fclose(write_file);
    munmap(file_memory, file_size);
    fclose(file);
}

// part 4
void diskput(int argc, char *argv[])
{
    // I did not do part 4
}

int main(int argc, char *argv[])
{
#if defined(PART1)
    diskinfo(argc, argv);
#elif defined(PART2)
    disklist(argc, argv);
#elif defined(PART3)
    diskget(argc, argv);
#elif defined(PART4)
    diskput(argc, argv);
#else
#error "argc[1234] must be defined"
#endif
    return 0;
}