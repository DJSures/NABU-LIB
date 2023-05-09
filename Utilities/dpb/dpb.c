#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpm.h>

struct dpb * dp;
int blk_size, mask_count;
int sec_count, cpm_ver;

void main() {

  puts("");
  puts("DPB Viewer              (c)2023 DJ Sures");
  puts("----------------------------------------");

  cpm_ver = bdos(CPM_VERS, 0);
  
  printf("CP/M version: %x.%x\n", cpm_ver >> 4, cpm_ver & 0xf);

  uint8_t volume = get_current_volume();

  printf("Volume: %c\n", 'A' + volume);

  uint16_t * dph = (uint16_t *)biosh(9, volume, 0);

  printf("DPH is at 0x%x\n", dph);
  
  uint8_t inc = 5;

  if (cpm_ver >= 3)
    inc = 6;

  uint16_t *dphBase = dph + inc;

  printf("DPB is at: 0x%x\n", (uint16_t)*dphBase);

  dp = (void *)*dphBase;

  printf("Sectors per Track (SPT)..%u",dp->SPT);

  if (dp->SPT == 26)
    printf(" 8\"\n");
  else
    printf("\n");

  printf("Block Shift (BSH)........%u\n",dp->BSH);
  printf("Block Mask (BLM).........%u\n",dp->BLM);
  printf("Extent Mask (EXM)........%u\n",dp->EXM);
  printf("Total Blocks (DSM).......%u\n",dp->DSM);
  printf("Directory Entries (DRM)..%u\n",dp->DRM);
  printf("Allocation 0 (AL0).......%Xh\n",dp->AL0);
  printf("Allocation 1 (AL1).......%Xh\n",dp->AL1);
  printf("Dir chk vector sz (CKS)..%u\n",dp->CKS);
  printf("Reserved Offset (OFF)....%u\n",dp->OFF);
  
  if (*dph == NULL) {

    printf("No software interleave\n\n");
  } else {

    printf("Skew %xh.\n\n", (uint16_t *)*dph);

    puts("-- Show Interleave Table? [y/n] --");

    while (true) {

      uint8_t i = getk();

      if (i == 'y')
        break;

      if (i == 'n')
        return;
    }

    uint8_t * xltab = (void *) *dph;

    for (sec_count = 0; sec_count < dp->SPT; sec_count++)
      printf("%u ",xltab[sec_count]);

    printf("\n\n");
  }

  if (dp->DSM < 256)
    blk_size=1024;
  else
    blk_size=2048;

  for (mask_count = dp->EXM+1; mask_count /= 2; mask_count>=0) 
    blk_size *= 2;

  if (blk_size != 128 << dp->BSH)
    printf("(warning: block size could also be %u).\n", 128 << dp->BSH);

  printf("Formatted capacity: %lu.\n", (long)blk_size*((long)dp->DSM+1));
  printf("Block (extent) size: %u.\n", blk_size);
  printf("MAX directory entries: %u, %u per block (%u blocks used).\n", dp->DRM+1, blk_size/32, (dp->DRM+1)/(blk_size/32));
}

