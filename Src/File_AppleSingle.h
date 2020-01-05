/**
 * AppleSingle file format support
 * RFC 1740: https://tools.ietf.org/html/rfc1740
 *
 * Author: David Stancu, @mach-kernel, Mar. 2018
 *
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "Dc_Shared.h"
#include "Dc_Prodos.h"

const unsigned static int AS_MAGIC;

#pragma pack(push, 1)

typedef struct as_file_header
{
  DWORD magic;
  DWORD version;
  DWORD filler[4];
  WORD num_entries;
} as_file_header;

typedef struct as_file_entry
{
  DWORD entry_id;
  DWORD offset;
  DWORD length;
} as_file_entry;

typedef enum as_entry_types
{
  data_fork = 1,
  resource_fork = 2,
  real_name = 3,
  comment = 4,
  icon_bw = 5,
  icon_color = 6,
  file_dates_info = 8,
  finder_info = 9,
  mac_file_info = 10,
  prodos_file_info = 11,
  msdos_file_info = 12,
  short_name = 13,
  afp_file_info = 14,
  directory_id = 15
} as_entry_types;

typedef struct as_prodos_info 
{
  WORD access;
  WORD filetype;
  DWORD auxtype;
} as_prodos_info;

typedef struct as_from_prodos
{
  uint16_t length;
  unsigned char *data;
} as_from_prodos;

#pragma pack(pop)

bool ASIsAppleSingle(unsigned char *buf);

struct as_file_header *ASParseHeader(unsigned char *buf);
struct as_prodos_info *ASParseProdosEntry(unsigned char *entry_buf);
struct as_file_entry *ASGetEntries(struct as_file_header *header, unsigned char *buf);

void ASDecorateDataFork(struct prodos_file *current_file, unsigned char *data, as_file_entry *data_fork_entry);
void ASDeocrateProdosFileInfo(struct prodos_file *current_file, unsigned char *data, as_file_entry *prodos_entry);
void ASDecorateProdosFile(struct prodos_file *current_file, unsigned char *data);

struct as_from_prodos ASFromProdosFile(struct prodos_file *file);
