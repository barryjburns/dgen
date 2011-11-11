// DGen v1.06+
// MYFM.CPP - interface to YM2612Update

#include <stdio.h>
#include <stdlib.h>
#include "md.h"

// md.h also has include fm.h

// REMEMBER NOT TO USE ANY STATIC variables, because they
// will exist thoughout ALL megadrives!

int md::myfm_write(int a,int v,int md)
{
  int sid=0;
  int mame_can_have_it=1;

  (void)md;

  a&=3;

  sid=(a&2)?1:0;

  if ((a&1)==0) fm_sel[sid]=v;
  else
  {
    // stash all values
    fm_reg[sid] [fm_sel[sid]&0xff] = v;

    if (fm_sel[sid]==0x2a) { dac_submit(v); mame_can_have_it=0; }
    if (fm_sel[sid]==0x2b) { dac_enable(v); mame_can_have_it=0; }

    if ((fm_sel[sid]&0xfc)==0x24) { mame_can_have_it=0; }

    if (fm_sel[sid]==0x27)
    {
      if (v&0x10) fm_tover[0]&=~1;
      if (v&0x20) fm_tover[0]&=~2;
      // timer ack?
    }

  }

  if (mame_can_have_it)
    YM2612Write(0,a&3,v);

  return 0;
}

int md::myfm_read(int a)
{
  if ((a&1)==0)
    return fm_tover[0];
  else
  {
    YM2612Read(0, (a & 3));
  }
  return 0;
}

int md::mysn_write(int d)
{
  SN76496Write(0, d);
  return 0;
}

// This called once per raster
int md::fm_timer_callback()
{
  int amax,bmax, i;

  // Our raster lasts 63.61323 microseconds
  for (i=0;i<4;i++) ras_fm_ticker[i]+=64;

//  if (sound_is_okay) ret=YM2612Read(0,a&3);

  // period of timer a is
  // 18 * (1024 - ((fm_reg[0][0x24]<<2)+(fm_reg[0][0x25])&3))) microsecs

  amax=18 * (1024 -  ((fm_reg[0][0x24]<<2)+(fm_reg[0][0x25]&3)) );
  bmax=288*(256-fm_reg[0][0x26]);

  if (bmax<=0) bmax=1;
  if (amax<=0) amax=1;

  if (ras_fm_ticker[0]>=amax)
  {
    fm_tover[0]|=1; ras_fm_ticker[0]-=amax;
  }

  if (ras_fm_ticker[1]>=bmax)
  {
    fm_tover[0]|=2; ras_fm_ticker[1]-=bmax;
  }
  return 0;
}
