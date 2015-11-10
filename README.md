# Disk Image Browser (dib)
The Disk Image Browser project aims to create a new module for Islandora that will allow for the contents of a disk image to be ingested along with the original disk image. This will allow users to browse the contents of the disk image in browser instead of requiring them to download the disk image and find a way to open it themselves.

#Current Progress
A C program has been written that can copy the contents of a given disk image into a user-specified folder. This program currently runs outside of Islandora as an individual executable.

##Running the C Program
- Make sure [https://github.com/sleuthkit/sleuthkit](The Sleuth Kit) is installed.  
- Clone this repository
- From command line, make the executable dib file by calling 

>make

- Run the dib executable:

>[Path to executable]/dib [path to disk image] [output file name]

- The output file will be found in the current directory

The current goal is to find a way to integrate this code into Islandora to fulfill the main objective of this project.

The developers are currently looking into using the swig tool to port the C program into PHP code that can be integrated into Islandora.

#Dependencies
The program depends on The Sleuth Kit (4.1.3 or later) library.
