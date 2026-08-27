#ifndef ATA_H
#define ATA_H

#include <stdbool.h>
#include <stdint.h>

// Driver data structure inferred from assembly offsets
typedef struct __attribute__((packed)) {
  uint32_t prtlen;
  uint16_t dcr;
  uint32_t stLBA;
  uint16_t tf;
  uint8_t sbits;
} driverdata_t;

bool write_ata_st(int32_t *sectors, const uint16_t **source, driverdata_t *dd,
                  uint32_t *lba);
bool read_ata_st(int32_t *sectors, uint16_t **dest, driverdata_t *dd,
                 uint32_t *lba);

#endif
