#!/usr/bin/env python3

# Create a new header file including all the .h file in a directory

import os
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a new header file including all the .h file in a directory")
    parser.add_argument("-d", "--directory", help="Directory to scan", required=True)
    parser.add_argument("-p", "--prefix", help="Prefix to add to the header file", required=True)
    args = parser.parse_args()

    # Get all the .h files
    h_files = []
    for root, dirs, files in os.walk(args.directory):
        for file in files:
            if file.endswith(".h"):
                h_files.append(os.path.join(args.prefix, file))

    # Create the header file
    print("#pragma once")
    print("")
    for h_file in h_files:
        print("#include <{}>".format(h_file))
    print("")
