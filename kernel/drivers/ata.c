#include <drivers/ata.h>
#include <kernel/klib.h>

// do a singletasking PIO ata "software reset" with DCR
void srst_ata_st(uint16_t dcr) {
  outb(dcr, 0x04); // do a "software reset" on the bus
  outb(dcr, 0x00); // reset the bus to normal operation

  // 400ns delay to allow status bits to reset
  inb(dcr);
  inb(dcr);
  inb(dcr);
  inb(dcr);

  // check BSY and RDY (want BSY clear and RDY set)
  while ((inb(dcr) & 0xC0) != 0x40) {
    // block until drive is ready
  }
}

// ATA PIO 28-bit singletasking disk read function (up to 256 sectors)
bool pio28_read(uint32_t *sectors, uint16_t **dest, driverdata_t *dd,
                uint32_t *lba) {
  uint32_t abs_lba = *lba + dd->stLBA;
  uint16_t port = dd->tf;

  outb(port + 2, (uint8_t)*sectors);
  outb(port + 3, (uint8_t)abs_lba);
  outb(port + 4, (uint8_t)(abs_lba >> 8));
  outb(port + 5, (uint8_t)(abs_lba >> 16));
  outb(port + 6, ((uint8_t)(abs_lba >> 24) & 0x0F) | dd->sbits);
  outb(port + 7, 0x20); // "read" command

  // implement 400ns delay on ERR only; wait for BSY clear and DRQ set
  for (int i = 0; i < 4; i++) {
    uint8_t status = inb(port + 7);
    if (!(status & 0x80) && (status & 0x08))
      goto data_rdy;
  }

pior_l:
  while (1) {
    uint8_t status = inb(port + 7);
    if (status & 0x80)
      continue; // BSY flag set
    if (status & 0x21)
      return false; // ERR or DF set
    break;
  }

data_rdy:
  insw(port, *dest, 256); // gulp one 512b sector (256 words)
  *dest += 256;           // advance pointer

  // delay 400ns to allow drive to set new values of BSY and DRQ
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);

  (*lba)++;
  (*sectors)--;

  // check if the low byte just turned 0 (more sectors to read?)
  if ((uint8_t)*sectors != 0)
    goto pior_l;

  return (inb(port + 7) & 0x21) == 0; // test the last status ERR bits
}

// ATA PIO 48-bit singletasking disk read function (up to 64K sectors)
bool pio48_read(uint32_t *sectors, uint16_t **dest, driverdata_t *dd,
                uint32_t *lba) {
  uint64_t abs_lba = (uint64_t)*lba + dd->stLBA;
  uint16_t port = dd->tf;

  outb(port + 2, (uint8_t)(*sectors >> 8)); // sectorcount high
  outb(port + 3, (uint8_t)(abs_lba >> 24)); // LBA4
  outb(port + 4, (uint8_t)(abs_lba >> 32)); // LBA5
  outb(port + 5, 0);                        // LBA6 always 0
  outb(port + 2, (uint8_t)*sectors);        // sectorcount low
  outb(port + 3, (uint8_t)abs_lba);         // LBA1
  outb(port + 4, (uint8_t)(abs_lba >> 8));  // LBA2
  outb(port + 5, (uint8_t)(abs_lba >> 16)); // LBA3

  outb(port + 6, (dd->sbits & 0x50)); // drive select (strip LBA28 bits)
  outb(port + 7, 0x24);               // "read ext" command

  for (int i = 0; i < 4; i++) {
    uint8_t status = inb(port + 7);
    if (!(status & 0x80) && (status & 0x08))
      goto data_rdy;
  }

pior_l:
  while (1) {
    uint8_t status = inb(port + 7);
    if (status & 0x80)
      continue;
    if (status & 0x21)
      return false;
    break;
  }

data_rdy:
  insw(port, *dest, 256);
  *dest += 256;

  inb(port + 7);
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);

  (*lba)++;
  (*sectors)--;

  // check if "sectorcount" just decremented to 0
  if ((uint16_t)*sectors != 0)
    goto pior_l;

  return (inb(port + 7) & 0x21) == 0;
}

// Main singletasking PIO ATA read entry point
// returns true (Zero flag set) on success, false (Carry set) on failure
bool read_ata_st(int32_t *sectors, uint16_t **dest, driverdata_t *dd,
                 uint32_t *lba) {
  if (*sectors < 0) {
    srst_ata_st(dd->dcr);
    *sectors = 0;
    return true;
  }

  if (*sectors > 0x3fffff)
    return false;

  // verify boundaries against partition limits
  uint32_t max_sectors = dd->prtlen - 1;
  if (*lba > max_sectors)
    return false;
  if ((uint32_t)*sectors > max_sectors)
    return false;
  if (*lba > (max_sectors - *sectors + 1))
    return false;

  // check the bsy and drq bits -- both must be clear
  if (inb(dd->dcr) & 0x88)
    return false;

  // if lba > 28bit or stlba > 28bit or total exceeds 28bit, use 48 bit
  uint64_t bounds_check = (uint64_t)*lba + dd->stLBA + *sectors;
  bool use_48bit = (bounds_check > 0xfffffff);

  while (*sectors > 0) {
    bool success = use_48bit ? pio48_read((uint32_t *)sectors, dest, dd, lba)
                             : pio28_read((uint32_t *)sectors, dest, dd, lba);

    if (!success)
      return false;
  }

  return true;
}

// ATA PIO 28-bit singletasking disk write function (up to 256 sectors)
bool pio28_write(uint32_t *sectors, const uint16_t **src, driverdata_t *dd,
                 uint32_t *lba) {
  uint32_t abs_lba = *lba + dd->stLBA;
  uint16_t port = dd->tf;

  outb(port + 2, (uint8_t)*sectors);
  outb(port + 3, (uint8_t)abs_lba);
  outb(port + 4, (uint8_t)(abs_lba >> 8));
  outb(port + 5, (uint8_t)(abs_lba >> 16));
  outb(port + 6, ((uint8_t)(abs_lba >> 24) & 0x0F) | dd->sbits);

  outb(port + 7, 0x30); // "write sectors" command (0x30)

  // implement 400ns delay on ERR only; wait for BSY clear and DRQ set
  for (int i = 0; i < 4; i++) {
    uint8_t status = inb(port + 7);
    if (!(status & 0x80) && (status & 0x08))
      goto data_rdy;
  }

pior_l:
  while (1) {
    uint8_t status = inb(port + 7);
    if (status & 0x80)
      continue; // BSY flag set
    if (status & 0x21)
      return false; // ERR or DF set
    break;
  }

data_rdy:
  outsw(port, *src, 256); // blast one 512b sector (256 words) to the drive
  *src += 256;            // advance memory pointer

  // delay 400ns to allow drive to set new values of BSY and DRQ while it writes
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);

  (*lba)++;
  (*sectors)--;

  // check if the low byte just turned 0 (more sectors to write?)
  if ((uint8_t)*sectors != 0)
    goto pior_l;

  return (inb(port + 7) & 0x21) == 0; // test the last status ERR bits
}

// ATA PIO 48-bit singletasking disk write function (up to 64K sectors)
bool pio48_write(uint32_t *sectors, const uint16_t **src, driverdata_t *dd,
                 uint32_t *lba) {
  uint64_t abs_lba = (uint64_t)*lba + dd->stLBA;
  uint16_t port = dd->tf;

  outb(port + 2, (uint8_t)(*sectors >> 8)); // sectorcount high
  outb(port + 3, (uint8_t)(abs_lba >> 24)); // LBA4
  outb(port + 4, (uint8_t)(abs_lba >> 32)); // LBA5
  outb(port + 5, 0);                        // LBA6 always 0
  outb(port + 2, (uint8_t)*sectors);        // sectorcount low
  outb(port + 3, (uint8_t)abs_lba);         // LBA1
  outb(port + 4, (uint8_t)(abs_lba >> 8));  // LBA2
  outb(port + 5, (uint8_t)(abs_lba >> 16)); // LBA3

  outb(port + 6, (dd->sbits & 0x50)); // drive select (strip LBA28 bits)

  outb(port + 7, 0x34); // "write sectors ext" command (0x34)

  for (int i = 0; i < 4; i++) {
    uint8_t status = inb(port + 7);
    if (!(status & 0x80) && (status & 0x08))
      goto data_rdy;
  }

pior_l:
  while (1) {
    uint8_t status = inb(port + 7);
    if (status & 0x80)
      continue;
    if (status & 0x21)
      return false;
    break;
  }

data_rdy:
  outsw(port, *src, 256);
  *src += 256;

  inb(port + 7);
  inb(port + 7);
  inb(port + 7);
  inb(port + 7);

  (*lba)++;
  (*sectors)--;

  // check if "sectorcount" just decremented to 0
  if ((uint16_t)*sectors != 0)
    goto pior_l;

  return (inb(port + 7) & 0x21) == 0;
}

bool write_ata_st(int32_t *sectors, const uint16_t **source, driverdata_t *dd,
                  uint32_t *lba) {
  if (*sectors < 0) {
    srst_ata_st(dd->dcr);
    *sectors = 0;
    return true;
  }

  if (*sectors > 0x3fffff)
    return false;

  // verify boundaries against partition limits
  uint32_t max_sectors = dd->prtlen - 1;
  if (*lba > max_sectors)
    return false;
  if ((uint32_t)*sectors > max_sectors)
    return false;
  if (*lba > (max_sectors - *sectors + 1))
    return false;

  // check the bsy and drq bits -- both must be clear
  if (inb(dd->dcr) & 0x88)
    return false;

  // if lba > 28bit or stlba > 28bit or total exceeds 28bit, use 48 bit
  uint64_t bounds_check = (uint64_t)*lba + dd->stLBA + *sectors;
  bool use_48bit = (bounds_check > 0xfffffff);

  while (*sectors > 0) {
    bool success = use_48bit
                       ? pio48_write((uint32_t *)sectors, source, dd, lba)
                       : pio28_write((uint32_t *)sectors, source, dd, lba);

    if (!success)
      return false;
  }

  return true;
}
