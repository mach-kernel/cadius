#include "File_AppleSingle.h"

const unsigned char AS_MAGIC[] = {0x00, 0x05, 0x16, 0x00};

/**
 * Is this an AppleSingle file?
 * @brief ASIsAppleSingle
 * @param buf
 * @return
 */
bool ASIsAppleSingle(unsigned char *buf)
{
  return memcmp(&buf, &AS_MAGIC, sizeof(AS_MAGIC));
}

/**
 * Parse header out of raw data buffer.
 * @brief ASParseHeader
 * @param buf The buffer
 * @return
 */
struct as_file_header *ASParseHeader(unsigned char *buf)
{
  struct as_file_header *header = malloc(sizeof(as_file_header));
  memcpy(header, buf, sizeof(as_file_header));

  if (IS_LITTLE_ENDIAN) {
    header->magic = swap32(header->magic);
    header->version = swap32(header->version);
    header->num_entries = swap16(header->num_entries);
  }

  return header;
}

/**
 * @brief ASParseProdosEntry
 * @param entry_buf  The entry buffer
 * @return
 */
struct as_prodos_info *ASParseProdosEntry(unsigned char *entry_buf, DWORD length)
{
  struct as_prodos_info *prodos_entry = malloc(sizeof(as_prodos_info));
  memcpy(prodos_entry, entry_buf, length);

  if (IS_LITTLE_ENDIAN) 
  {
    prodos_entry->access = swap16(prodos_entry->access);
    prodos_entry->filetype = swap16(prodos_entry->filetype);
    prodos_entry->auxtype = swap32(prodos_entry->auxtype);
  }

  return prodos_entry;
}

/**
 * Read headers and return a list of as_file_entry pointers.
 * @brief ASGetEntries
 * @param buf
 * @return
 */
struct as_file_entry *ASGetEntries(struct as_file_header *header, unsigned char *buf)
{
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

  if (IS_LITTLE_ENDIAN)
    for (int i = 0; i < header->num_entries; ++i)
    {
      entries[i].entry_id = swap32(entries[i].entry_id);
      entries[i].offset = swap32(entries[i].offset);
      entries[i].length = swap32(entries[i].length);
    }

  return entries;
}

/**
 * Grab data from data entry and place in prodos_file.
 * @brief ASDecorateDataFork
 * @param current_file     The current file
 * @param data             The data
 * @param data_fork_entry  The data fork entry
 */
void ASDecorateDataFork(struct prodos_file *current_file, unsigned char *data, as_file_entry *data_fork_entry)
{
  if (data_fork_entry->entry_id != data_fork) return;

  unsigned char *data_entry = malloc(data_fork_entry->length);
  memcpy(data_entry, data + data_fork_entry->offset, data_fork_entry->length);
  current_file->data = data;

  return;
}

/**
 * Read ProDOS metadata struct and place in prodos_file.
 *
 * @brief ASDecorateProdosFileInfo
 * @param current_file  The current file
 * @param data          The data
 * @param prodos_entry  The prodos entry
 */
void ASDecorateProdosFileInfo(struct prodos_file *current_file, unsigned char *data, as_file_entry *prodos_entry)
{
  if (prodos_entry->entry_id != prodos_file_info) return;

  struct as_prodos_info *info_meta = ASParseProdosEntry(
    data + prodos_entry->offset, prodos_entry->length
  );

  if (!info_meta) return;

  current_file->access = info_meta->access;
  current_file->type = info_meta->filetype;
  current_file->aux_type = info_meta->auxtype;

  return;
}

/**
 * Parse AppleSingle header and write attributes into prodos_file
 * struct
 * @brief ASDecorateProdosFile
 * @param current_file
 * @param data
 */
void ASDecorateProdosFile(struct prodos_file *current_file, unsigned char *data)
{
    struct as_file_header *header = ASParseHeader(data);
    struct as_file_entry *entries = ASGetEntries(header, data);

    for (int i = 0; i < header->num_entries; ++i)
      switch(entries[i].entry_id)
      {
        case data_fork:
          ASDecorateDataFork(current_file, data, &entries[i]);
          break;
        case prodos_file_info:
          ASDecorateProdosFileInfo(current_file, data, &entries[i]);
          break;
        default:
          printf("        Entry ID %d unsupported, ignoring!\n", entries[i].entry_id);
          printf("        (See https://tools.ietf.org/html/rfc1740 for ID lookup)\n");
          break;
      }
    return;
}
