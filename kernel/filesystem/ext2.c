#include <drivers/ata.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint16_t sector[256];
driverdata_t qemu_disk = {
    .tf = 0x1f0, .dcr = 0x3f6, .sbits = 0xe0, .stLBA = 0, .prtlen = 131072};

void test_disk() {
  int32_t sector_count = 1;
  uint32_t lba_num = 0;
  uint16_t *p = sector;
  const uint16_t *cnst_p = sector;

  read_ata_st(&sector_count, &p, &qemu_disk, &lba_num);
  char *text = (char *)sector;
  if (strcmp("PROJECTOS_ATA_DRIVER_SUCCESS", text) == 0) {
    printf("read success\n");
  } else {
    printf("nope way off i had :%s\n", text);
  }
  strcat(text, ":MODIFIED");
  int some_shit_i_wont_use = memcmp(sector, text, strlen(text));
  sector_count = 1;
  lba_num = 0;
  write_ata_st(&sector_count, &cnst_p, &qemu_disk, &lba_num);
  sector_count = 1;
  lba_num = 0;
  read_ata_st(&sector_count, &p, &qemu_disk, &lba_num);
  text = (char *)sector;
  printf("and after the modification : %s", text);
}
