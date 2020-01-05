#include "File_AppleSingle.h"
#include "log.h"
#include "os/os.h"

const unsigned static int AS_MAGIC = (uint32_t) 0x00051600;

/**
 * Is this an AppleSingle file?
 * @brief ASIsAppleSingle
 * @param buf
 * @return
 */
bool ASIsAppleSingle(unsigned char *buf)
{
  int buf_magic;
  memcpy(&buf_magic, buf, sizeof(AS_MAGIC));
  buf_magic = ntohl(buf_magic);

  return buf_magic == AS_MAGIC;
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
  struct as_file_header *buf_header = (as_file_header *) buf;

  header->magic = ntohl(buf_header->magic);
  header->version = ntohl(buf_header->version);
  header->num_entries = ntohs(buf_header->num_entries);

  return header;
}

/**
 * @brief ASParseProdosEntry
 * @param entry_buf  The entry buffer
 * @return
 */
struct as_prodos_info *ASParseProdosEntry(unsigned char *entry_buf)
{
  struct as_prodos_info *prodos_entry = malloc(sizeof(as_prodos_info));
  struct as_prodos_info *buf_prodos_entry = (as_prodos_info *) entry_buf;

  prodos_entry->access = ntohs(buf_prodos_entry->access);
  prodos_entry->filetype = ntohs(buf_prodos_entry->filetype);
  prodos_entry->auxtype = ntohl(buf_prodos_entry->auxtype);

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
    logf_error("      Error: Invalid AppleSingle file!\n");
    return NULL;
  }

  struct as_file_entry *entries = malloc(
    header->num_entries * sizeof(as_file_entry)
  );

  struct as_file_entry *buf_entries = (as_file_entry *) (buf + sizeof(as_file_header));
  memcpy(entries, buf_entries, header->num_entries * sizeof(as_file_entry));

  for (int i = 0; i < header->num_entries; ++i)
  {
    entries[i].entry_id = ntohl(entries[i].entry_id);
    entries[i].offset = ntohl(entries[i].offset);
    entries[i].length = ntohl(entries[i].length);
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
  current_file->data = data_entry;
  current_file->data_length = data_fork_entry->length;

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
    data + prodos_entry->offset
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
          logf_info("        Entry ID %d unsupported, ignoring!\n", entries[i].entry_id);
          logf_info("        (See https://tools.ietf.org/html/rfc1740 for ID lookup)\n");
          break;
      }
    return;
}

struct as_from_prodos ASFromProdosFile(struct prodos_file *file)
{
  struct as_file_header *as_header = malloc(sizeof(as_file_header));
  as_header->magic = ntohl(AS_MAGIC);
  for (int i = 0; i < 4; ++i) as_header->filler[i] = 0;
  as_header->version = ntohl(0x00020000);
  as_header->num_entries = ntohs(2);

  uint32_t header_offset = sizeof(as_file_header) + (2 * sizeof(as_file_entry));
  struct as_file_entry *data_entry = malloc(sizeof(as_file_entry));
  data_entry->entry_id = ntohl(data_fork);
  data_entry->length = ntohl(file->data_length);
  data_entry->offset = ntohl(header_offset);

  uint32_t prodos_entry_offset = header_offset + file->data_length;
  struct as_file_entry *prodos_entry = malloc(sizeof(as_file_entry));
  prodos_entry->entry_id = ntohl(prodos_file_info);
  prodos_entry->length = ntohl(sizeof(as_prodos_info));
  prodos_entry->offset = ntohl(prodos_entry_offset);

  struct as_prodos_info *prodos_info = malloc(sizeof(as_prodos_info));
  prodos_info->access = ntohs(file->entry->access);
  prodos_info->filetype = ntohs(file->entry->file_type);
  prodos_info->auxtype = ntohl(file->entry->file_aux_type);

  uint32_t payload_size = prodos_entry_offset + sizeof(as_prodos_info);
  unsigned char *payload = malloc(payload_size);
  unsigned char *seek = payload;

  memcpy(seek, as_header, sizeof(as_file_header));
  seek += sizeof(as_file_header);

  memcpy(seek, data_entry, sizeof(as_file_entry));
  seek += sizeof(as_file_entry);

  memcpy(seek, prodos_entry, sizeof(as_file_entry));
  seek += sizeof(as_file_entry);

  memcpy(seek, file->data, file->data_length);
  seek += file->data_length;

  memcpy(seek, prodos_info, sizeof(as_prodos_info));

  struct as_from_prodos as_file; 
  as_file.length = payload_size;
  as_file.data = payload;

  return as_file;
}
