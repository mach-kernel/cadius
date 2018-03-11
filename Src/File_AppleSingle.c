#include "File_AppleSingle.h"

const unsigned char AS_MAGIC[] = {0x00, 0x05, 0x16, 0x00};

/**
 * Is this an AppleSingle file?
 * @brief IsAppleSingle
 * @param buf
 * @return
 */
bool IsAppleSingle(unsigned char *buf)
{
    return memcmp(&buf, &AS_MAGIC, sizeof(AS_MAGIC));
}

/**
 * Read headers and return a list of as_file_entry pointers
 * @brief GetEntries
 * @param buf
 * @return
 */
struct as_file_entry **GetEntries(unsigned char *buf)
{
    struct as_file_entry **entries = malloc(4 * sizeof(as_file_entry *));
    return entries;
}

/**
 * Parse AppleSingle header and write attributes into prodos_file
 * struct
 * @brief DecorateProdosFile
 * @param current_file
 * @param data
 */
void DecorateProdosFile(struct prodos_file *current_file, unsigned char *data)
{
    return;
}
