from pathlib import Path

def ascii_to_utf8_mapping():
    ascii_utf8_map = {}

    for ascii_value in range(256):  # ASCII values from 0 to 255
        char = chr(ascii_value)
        utf8_rep = char.encode('utf-8')  # UTF-8 encoding
        utf8_hex_rep = " ".join(f"0x{byte:02X}" for byte in utf8_rep)  # Convert to hex representation
        ascii_utf8_map[char] = utf8_hex_rep

    return ascii_utf8_map

def display_ascii_utf8_map():
    mapping = ascii_to_utf8_mapping()
    
    for char, utf8 in mapping.items():
        if char.isprintable():
            print(f"'{char}' (ASCII 0x{ord(char):02X}) -> {utf8}")
        else:
            print(f"    (ASCII 0x{ord(char):02X}) -> {utf8}")

# Run the display function to print the mapping
# display_ascii_utf8_map()


def ascii_to_utf8_lookup_table():
    # Create the lookup table that only handles the extended ASCII range 128-255
    lookup_table = []

    for ascii_value in range(128, 256):  # Extended ASCII values from 128 to 255
        char = chr(ascii_value)
        utf8_rep = char.encode('utf-8')  # UTF-8 encoding

        # UTF-8 for 128-255 is 2 bytes, ensure we are getting valid results
        if len(utf8_rep) == 2:
            # Add the tuple of (utf8_byte1, utf8_byte2, extended_ascii_value)
            lookup_table.append((utf8_rep[0], utf8_rep[1], ascii_value))

    return lookup_table


def generate_cpp_lookup_table(lookup_table):
    # Create the C++ header content
    header_content = """
/*
 * This table maps 2-byte UTF-8 sequences to their corresponding extended ASCII characters.
 * UTF-8 sequences are used for characters with ASCII values from 128 to 255 (extended ASCII).
 * 
 * Format:
 * {0xC2/0xC3, <second_byte>} -> <extended ASCII character>
 */

#ifndef UTF8_TO_ASCII_H
#define UTF8_TO_ASCII_H

#include <Arduino.h>

// Define the lookup table for UTF-8 to extended ASCII conversion
const uint8_t utf8_to_ascii[128][2] = {
"""

    # Loop over the lookup table and format each entry for the C++ array
    for utf8_byte1, utf8_byte2, ascii_value in lookup_table:
        header_content += f"    {{0x{utf8_byte1:02X}, 0x{utf8_byte2:02X}}}, // {utf8_byte1:3d}, {utf8_byte2:3d} -> 0x{ascii_value:02X} ('{chr(ascii_value)}')\n"

    header_content += """};

#endif // UTF8_TO_ASCII_H
"""
    return header_content


def save_cpp_header_file(content, filename="utf8_to_ascii.h"):
    # Save the content to a .h file
    with open(filename, "w") as file:
        file.write(content)


# Generate the lookup table
lookup_table = ascii_to_utf8_lookup_table()

# Generate the C++ header content
cpp_header_content = generate_cpp_lookup_table(lookup_table)

# Save to a .h file
save_cpp_header_file(cpp_header_content, Path(__file__).parent / "utf8_to_ascii.h")

print("C++ header file 'utf8_to_ascii.h' has been created.")

