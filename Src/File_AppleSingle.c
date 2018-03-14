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
 * Parse header out of raw data buffer.
 * @brief ParseHeader
 * @param buf The buffer
 * @return
 */
struct as_file_header *ParseHeader(unsigned char *buf)
{
  struct as_file_header *header = malloc(sizeof(as_file_header));
  memcpy(header, buf, sizeof(as_file_header));

  if (__ORDER_LITTLE_ENDIAN__) {
    header->magic = __builtin_bswap32(header->magic);
    header->version = __builtin_bswap32(header->version);
    header->num_entries = __builtin_bswap16(header->num_entries);
  }

  return header;
}

/**
 * Read headers and return a list of as_file_entry pointers
 * @brief GetEntries
 * @param buf
 * @return
 */
struct as_file_entry *GetEntries(unsigned char *buf)
{
  struct as_file_header *header = ParseHeader(buf);

  if (!header)
  {
    printf("      Invalid AppleSingle file!\n");
    return NULL;
  }

  struct as_file_entry *entries = malloc(
    header->num_entries * sizeof(as_file_entry)
  );

  struct as_file_entry *buf_entries = buf + sizeof(as_file_header);
  memcpy(entries, buf_entries, header->num_entries * sizeof(as_file_entry));

  if (__ORDER_LITTLE_ENDIAN__)
      for (int i = 0; i < header->num_entries; ++i)
      {
          entries[i].entry_id = __builtin_bswap32(entries[i].entry_id);
          entries[i].offset = __builtin_bswap32(entries[i].offset);
          entries[i].length = __builtin_bswap32(entries[i].length);
      }

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
    struct as_file_entry *entries = GetEntries(data);
    for (int i = 0; i < 2; ++i) {
        printf("      Found entry with ID %04x\n", entries[i].entry_id);
    }

    return;
}
