#!/bin/bash

echo "Tworzenie dysku twardego"
./virtual_disk create mydisk.img
read -p "Press Enter to continue..."

echo "Kopiowanie świeżo utworzonych plików do dysku twardego"
echo "buffer" > buffer
echo "Test file two" > testfile2.txt
echo "Test file one two three" > testfile1.txt
./virtual_disk copy_to mydisk.img buffer
./virtual_disk copy_to mydisk.img testfile2.txt
./virtual_disk copy_to mydisk.img testfile1.txt
read -p "Press Enter to continue..."

echo "Listowanie zawartości wirtualnego dysku twardego"
./virtual_disk list mydisk.img
read -p "Press Enter to continue..."

echo "Kopiowanie jednego pliku z wirtualnego dysku o nazwie extracted_testfile2.txt"
./virtual_disk copy_from mydisk.img testfile2.txt extracted_testfile2.txt
read -p "Press Enter to continue..."

echo "Pokazanie użycia dysku"
./virtual_disk show_usage mydisk.img
read -p "Press Enter to continue..."

echo "Usuwanie pliku testfile2.txt z dysku"
./virtual_disk delete mydisk.img testfile2.txt
read -p "Press Enter to continue..."

echo "Listowanie ponownie zawartości wirtualnego dysku twardego"
./virtual_disk list mydisk.img
./virtual_disk show_usage mydisk.img
read -p "Press Enter to continue..."

echo "Usunięcie dysku twardego"
./virtual_disk delete_disk mydisk.img
read -p "Press Enter to continue..."

echo "Czyszczenie danych tymczasowych"
rm -f testfile1.txt testfile2.txt extracted_testfile2.txt buffer
