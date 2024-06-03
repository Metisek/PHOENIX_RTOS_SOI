#!/bin/bash

DISK_NAME="virtual_disk.img"
SMALL_FILE="small_file.txt"
LARGE_FILE="large_file.txt"
DEST_SMALL_FILE="copied_small_file.txt"
DEST_LARGE_FILE="copied_large_file.txt"

pause() {
    read -p "Press [SPACE] to continue..." -n 1 -r
    echo
}

# Create small and large source files for testing
echo "This is a small test file" > $SMALL_FILE
dd if=/dev/zero of=$LARGE_FILE bs=1M count=5
echo "Source files created."
pause

# Create virtual disk
./virtual_disk create $DISK_NAME
echo "Virtual disk created."
pause

# Copy small file to virtual disk
./virtual_disk copy_to $DISK_NAME $SMALL_FILE
echo "Small file copied to virtual disk."
pause

# Copy large file to virtual disk
./virtual_disk copy_to $DISK_NAME $LARGE_FILE
echo "Large file copied to virtual disk."
pause

# List files on virtual disk
./virtual_disk list $DISK_NAME
pause

# Copy small file from virtual disk
./virtual_disk copy_from $DISK_NAME $SMALL_FILE $DEST_SMALL_FILE
echo "Small file copied from virtual disk."
pause

# Copy large file from virtual disk
./virtual_disk copy_from $DISK_NAME $LARGE_FILE $DEST_LARGE_FILE
echo "Large file copied from virtual disk."
pause

# List files on virtual disk
./virtual_disk list $DISK_NAME
pause

# Show disk usage
./virtual_disk show_usage $DISK_NAME
pause

# Delete small file from virtual disk
./virtual_disk delete $DISK_NAME $SMALL_FILE
echo "Small file deleted from virtual disk."
pause

# Delete large file from virtual disk
./virtual_disk delete $DISK_NAME $LARGE_FILE
echo "Large file deleted from virtual disk."
pause

# Show disk usage
./virtual_disk show_usage $DISK_NAME
pause

# Delete virtual disk
./virtual_disk delete_disk $DISK_NAME
echo "Virtual disk deleted."
pause

# Clean up
rm $SMALL_FILE $LARGE_FILE $DEST_SMALL_FILE $DEST_LARGE_FILE
echo "Cleanup done."
pause
