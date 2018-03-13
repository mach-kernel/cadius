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
 * @param buf   The buffer
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
struct as_file_entry **GetEntries(unsigned char *buf)
{
  struct as_file_header *header = ParseHeader(buf);

  if (!header) {
    printf("  Invalid AppleSingle file!\n");
    return NULL;
  }

  struct as_file_entry **entries = malloc(
    header->num_entries * sizeof(as_file_entry *)
  );

  unsigned char *move_buf = buf + sizeof(as_file_header);

  for (int i = 0; i < header->num_entries; ++i)
  {
    struct as_file_entry *entry = malloc(sizeof(as_file_entry));

    memcpy(entry, move_buf, sizeof(as_file_entry));
    printf("This entry is %d", __builtin_bswap32(entry->entry_id));

    entries[i] = entry;
    buf += sizeof(as_file_entry);
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
    GetEntries(data);
    return;
}
