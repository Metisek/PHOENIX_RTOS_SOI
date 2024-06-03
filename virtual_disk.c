#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILENAME 255
#define MAX_FILES 100
#define BLOCK_SIZE 512
#define DISK_SIZE  (BLOCK_SIZE * 16)

typedef struct {
    char filename[MAX_FILENAME];
    size_t size;
    size_t start_block;
} FileEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    int file_count;
} Directory;

typedef struct {
    int is_free;
} Block;

typedef struct {
    Block blocks[DISK_SIZE / BLOCK_SIZE];
} BlockMap;

void create_virtual_disk(const char* disk_name, size_t size) {
    FILE *disk = fopen(disk_name, "wb");
    if (disk == NULL) {
        perror("Failed to create disk");
        exit(1);
    }
    ftruncate(fileno(disk), size);
    fclose(disk);

    Directory root;
    root.file_count = 0;

    disk = fopen(disk_name, "rb+");
    fseek(disk, 0, SEEK_SET);
    fwrite(&root, sizeof(Directory), 1, disk);
    fclose(disk);

    BlockMap map;
    for (int i = 0; i < DISK_SIZE / BLOCK_SIZE; i++) {
        map.blocks[i].is_free = 1;
    }

    disk = fopen(disk_name, "rb+");
    fseek(disk, sizeof(Directory), SEEK_SET);
    fwrite(&map, sizeof(BlockMap), 1, disk);
    fclose(disk);
}

void copy_to_virtual_disk(const char* disk_name, const char* src_file) {
    FILE *src = fopen(src_file, "rb");
    if (src == NULL) {
        perror("Failed to open source file");
        exit(1);
    }

    fseek(src, 0, SEEK_END);
    size_t file_size = ftell(src);
    fseek(src, 0, SEEK_SET);

    FILE *disk = fopen(disk_name, "rb+");
    if (disk == NULL) {
        perror("Failed to open disk");
        fclose(src);
        exit(1);
    }

    Directory root;
    fseek(disk, 0, SEEK_SET);
    fread(&root, sizeof(Directory), 1, disk);

    if (root.file_count >= MAX_FILES) {
        fprintf(stderr, "Directory is full\n");
        fclose(src);
        fclose(disk);
        return;
    }

    BlockMap map;
    fseek(disk, sizeof(Directory), SEEK_SET);
    fread(&map, sizeof(BlockMap), 1, disk);

    int start_block = -1;
    int contiguous_free_blocks = 0;

    for (int i = 0; i < DISK_SIZE / BLOCK_SIZE; i++) {
        if (map.blocks[i].is_free) {
            contiguous_free_blocks++;
            if (contiguous_free_blocks * BLOCK_SIZE >= file_size) {
                start_block = i - contiguous_free_blocks + 1;
                break;
            }
        } else {
            contiguous_free_blocks = 0;
        }
    }

    if (start_block == -1) {
        fprintf(stderr, "Not enough space on disk\n");
        fclose(src);
        fclose(disk);
        return;
    }

    for (int i = start_block; i < start_block + (file_size / BLOCK_SIZE) + 1; i++) {
        map.blocks[i].is_free = 0;
    }

    FileEntry new_file;
    strncpy(new_file.filename, src_file, MAX_FILENAME - 1);
    new_file.filename[MAX_FILENAME - 1] = '\0';
    new_file.size = file_size;
    new_file.start_block = start_block;

    root.files[root.file_count] = new_file;
    root.file_count++;

    fseek(disk, 0, SEEK_SET);
    fwrite(&root, sizeof(Directory), 1, disk);

    fseek(disk, sizeof(Directory), SEEK_SET);
    fwrite(&map, sizeof(BlockMap), 1, disk);

    fseek(disk, start_block * BLOCK_SIZE, SEEK_SET);
    char buffer[BLOCK_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, src)) > 0) {
        fwrite(buffer, 1, bytes_read, disk);
    }

    fclose(src);
    fclose(disk);
}

void copy_from_virtual_disk(const char* disk_name, const char* filename, const char* dest_file) {
    FILE *dest = fopen(dest_file, "wb");
    if (dest == NULL) {
        perror("Failed to open destination file");
        exit(1);
    }

    FILE *disk = fopen(disk_name, "rb");
    if (disk == NULL) {
        perror("Failed to open disk");
        fclose(dest);
        exit(1);
    }

    Directory root;
    fseek(disk, 0, SEEK_SET);
    fread(&root, sizeof(Directory), 1, disk);

    int file_index = -1;
    for (int i = 0; i < root.file_count; i++) {
        if (strcmp(root.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        fprintf(stderr, "File not found on virtual disk\n");
        fclose(dest);
        fclose(disk);
        return;
    }

    FileEntry file = root.files[file_index];

    fseek(disk, file.start_block * BLOCK_SIZE, SEEK_SET);
    char buffer[BLOCK_SIZE];
    size_t bytes_to_copy = file.size;
    size_t bytes_read;

    while (bytes_to_copy > 0) {
        if (bytes_to_copy >= BLOCK_SIZE)
            bytes_read = fread(buffer, 1, BLOCK_SIZE, disk);
        else
            bytes_read = fread(buffer, 1, bytes_to_copy, disk);

        fwrite(buffer, 1, bytes_read, dest);
        bytes_to_copy -= bytes_read;

        if (feof(disk)) // Obsługa końca pliku
            break;
    }

    fclose(dest);
    fclose(disk);
}


void list_virtual_disk(const char* disk_name) {
    FILE *disk = fopen(disk_name, "rb");
    if (disk == NULL) {
        perror("Failed to open disk");
        exit(1);
    }

    Directory root;
    fseek(disk, 0, SEEK_SET);
    fread(&root, sizeof(Directory), 1, disk);

    for (int i = 0; i < root.file_count; i++) {
        printf("Filename: %s, Size: %zu bytes, Start block: %zu\n",
            root.files[i].filename, root.files[i].size, root.files[i].start_block);
    }

    fclose(disk);
}

void delete_from_virtual_disk(const char* disk_name, const char* filename) {
    FILE *disk = fopen(disk_name, "rb+");
    if (disk == NULL) {
        perror("Failed to open disk");
        exit(1);
    }

    Directory root;
    fseek(disk, 0, SEEK_SET);
    fread(&root, sizeof(Directory), 1, disk);

    int file_index = -1;
    for (int i = 0; i < root.file_count; i++) {
        if (strcmp(root.files[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        fprintf(stderr, "File not found on virtual disk\n");
        fclose(disk);
        return;
    }

    FileEntry file = root.files[file_index];

    BlockMap map;
    fseek(disk, sizeof(Directory), SEEK_SET);
    fread(&map, sizeof(BlockMap), 1, disk);

    for (int i = file.start_block; i < file.start_block + (file.size / BLOCK_SIZE) + 1; i++) {
        map.blocks[i].is_free = 1;
    }

    for (int i = file_index; i < root.file_count - 1; i++) {
        root.files[i] = root.files[i + 1];
    }
    root.file_count--;

    fseek(disk, 0, SEEK_SET);
    fwrite(&root, sizeof(Directory), 1, disk);

    fseek(disk, sizeof(Directory), SEEK_SET);
    fwrite(&map, sizeof(BlockMap), 1, disk);

    fclose(disk);
}

void delete_virtual_disk(const char* disk_name) {
    if (remove(disk_name) != 0) {
        perror("Failed to delete disk");
        exit(1);
    }
}

void show_disk_usage(const char* disk_name) {
    FILE *disk = fopen(disk_name, "rb");
    if (disk == NULL) {
        perror("Failed to open disk");
        exit(1);
    }

    BlockMap map;
    fseek(disk, sizeof(Directory), SEEK_SET);
    fread(&map, sizeof(BlockMap), 1, disk);

    for (int i = 0; i < DISK_SIZE / BLOCK_SIZE; i++) {
        printf("Block %d: %s\n", i, map.blocks[i].is_free ? "Free" : "Occupied");
    }

    fclose(disk);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args]\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "create") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s create <disk_name>\n", argv[0]);
            return 1;
        }
        create_virtual_disk(argv[2], DISK_SIZE);
    } else if (strcmp(command, "copy_to") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s copy_to <disk_name> <src_file>\n", argv[0]);
            return 1;
        }
        copy_to_virtual_disk(argv[2], argv[3]);
    } else if (strcmp(command, "copy_from") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s copy_from <disk_name> <filename> <dest_file>\n", argv[0]);
            return 1;
        }
        copy_from_virtual_disk(argv[2], argv[3], argv[4]);
    } else if (strcmp(command, "list") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s list <disk_name>\n", argv[0]);
            return 1;
        }
        list_virtual_disk(argv[2]);
    } else if (strcmp(command, "delete") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s delete <disk_name> <filename>\n", argv[0]);
            return 1;
        }
        delete_from_virtual_disk(argv[2], argv[3]);
    } else if (strcmp(command, "delete_disk") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s delete_disk <disk_name>\n", argv[0]);
            return 1;
        }
        delete_virtual_disk(argv[2]);
    } else if (strcmp(command, "show_usage") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s show_usage <disk_name>\n", argv[0]);
            return 1;
        }
        show_disk_usage(argv[2]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
