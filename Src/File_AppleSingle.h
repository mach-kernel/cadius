/**
 * AppleSingle file format support
 *
 * Author: David Stancu, @mach-kernel, Mar. 2018
 *
 */

typedef struct as_file_header {
    DWORD magic;
    DWORD version;
    int filler[4];
    WORD num_entries;
}

typedef struct as_file_entry {
    DWORD entry_id;
    DWORD offset;
    DWORD length;
}

struct as_file_entry **GetEntries(unsigned char *buf);
unsigned char *GetDataSegment(unsigned char *buf);