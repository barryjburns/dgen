// DGen

#ifndef __MD_H__
#define __MD_H__

#define VER VERSION

#include <stdint.h>

#ifdef WITH_STAR
#ifndef __STARCPU_H__
#include "star/starcpu.h"
#endif
#endif

#ifdef WITH_M68KEM
#ifndef M68000__HEADER
extern "C" {
#include "m68000.h"
}
#endif
#endif

#ifdef WITH_MUSA
#ifndef M68K__HEADER
extern "C"
{
#include "musa/m68k.h"
}
#endif
#endif

#ifdef WITH_MZ80
#ifndef	_MZ80_H_
#include "mz80/mz80.h"
#endif
#endif

#ifdef WITH_CZ80
#include "cz80/cz80.h"
#endif

//#define BUILD_YM2612
extern "C" {
#include "fm.h"
}

#include "sn76496.h"

extern "C" int test_ctv(unsigned char *dest, int len);
extern "C" int blur_bitmap_16(unsigned char *dest, int len);
extern "C" int blur_bitmap_15(unsigned char *dest, int len);

struct bmap { unsigned char *data; int w,h; int pitch; int bpp; };

// New struct, happily encapsulates all the sound info
struct sndinfo {
	int16_t *lr;
	unsigned int len; /* number of stereo samples */
};

int get_md_palette(unsigned char pal[256],unsigned char *cram);

class md;
class md_vdp
{
public:
  // Next three lines are the state of the VDP
  // They have to be public so we can save and load states
  // and draw the screen.
  uint8_t mem[(0x10100 + 0x35)]; //0x20+0x10+0x4+1 for dirt
  uint8_t *vram, *cram, *vsram;
  uint8_t reg[0x20];
  int rw_mode,rw_addr,rw_dma;
private:
  int poke_vram (int addr,unsigned char d);
  int poke_cram (int addr,unsigned char d);
  int poke_vsram(int addr,unsigned char d);
  int dma_len();
  int dma_addr();
  unsigned char dma_mem_read(int addr);
  int putword(unsigned short d);
  int putbyte(unsigned char d);
  // Used by draw_scanline to render the different display components
  void draw_tile1(int which, int line, unsigned char *where);
  void draw_tile1_solid(int which, int line, unsigned char *where);
  void draw_tile2(int which, int line, unsigned char *where);
  void draw_tile2_solid(int which, int line, unsigned char *where);
  void draw_tile3(int which, int line, unsigned char *where);
  void draw_tile3_solid(int which, int line, unsigned char *where);
  void draw_tile4(int which, int line, unsigned char *where);
  void draw_tile4_solid(int which, int line, unsigned char *where);
  void draw_window(int line, int front);
  void draw_sprites(int line, int front);
  void draw_plane_back0(int line);
  void draw_plane_back1(int line);
  void draw_plane_front0(int line);
  void draw_plane_front1(int line);
  // Working variables for the above
  unsigned char sprite_order[0x101], *sprite_base;
  int sprite_count;
  unsigned int Bpp;
  unsigned int Bpp_times8;
  unsigned char *dest;
  md& belongs;
public:
  md_vdp(md&);
  ~md_vdp();
// These are called by MEM.CPP
  int command(unsigned int cmd);
  unsigned short readword();
  unsigned char readbyte();
  int writeword(unsigned short d);
  int writebyte(unsigned char d);

  unsigned char *dirt; // Bitfield: what has changed VRAM/CRAM/VSRAM/Reg
  void reset();

  uint32_t highpal[64];
  // Draw a scanline
  void draw_scanline(struct bmap *bits, int line);
};

/* Generic structures for dumping and restoring M68K and Z80 states. */

typedef struct {
	/* Stored MSB first (big endian) */
	uint32_t d[8]; /* D0-D7 */
	uint32_t a[8]; /* A0-A7 */
	uint32_t pc;
	uint16_t sr;
} m68k_state_t;

typedef struct {
	/* Stored LSB first (little endian) */
	struct {
		uint16_t fa;
		uint16_t cb;
		uint16_t ed;
		uint16_t lh;
	} alt[2]; /* [1] = alternate registers */
	uint16_t ix;
	uint16_t iy;
	uint16_t sp;
	uint16_t pc;
	uint8_t r;
	uint8_t i;
	uint8_t iff; /* x x x x x x IFF2 IFF1 */
	uint8_t im; /* interrupt mode */
} z80_state_t;

#ifdef WITH_M68KEM
// Make sure this gets C linkage
extern "C" void cpu_setOPbase24(int pc);
#endif

class md
{
private:
#ifdef WITH_M68KEM
  // Yuck - had to use friend function :(
  friend void cpu_setOPbase24(int pc);
#endif
  int romlen;
  int ok;
  unsigned char *mem,*rom,*ram,*z80ram;
  // Saveram stuff:
  unsigned char *saveram; // The actual saveram buffer
  unsigned save_start, save_len; // Start address and length
  int save_prot, save_active; // Flags set from $A130F1
public:
  md_vdp vdp;
  m68k_state_t m68k_state;
  z80_state_t z80_state;
  void m68k_state_dump();
  void m68k_state_restore();
  void z80_state_dump();
  void z80_state_restore();
private:
#ifdef WITH_MZ80
  struct mz80context z80;
#endif
#ifdef WITH_CZ80
  cz80_struc cz80;
  int cz80_cycles;
#endif
#ifdef WITH_STAR
  struct S68000CONTEXT cpu;
  STARSCREAM_PROGRAMREGION *fetch;
  STARSCREAM_DATAREGION    *readbyte,*readword,*writebyte,*writeword;
  int memory_map();
#endif
  int star_mz80_on();
  int star_mz80_off();

  int z80_bank68k; // 9 bits
  int z80_online;
  int odo; // value of odo now/at end of frame
  int ras;
  int z80_extra_cycles;

  // Note order is (0) Vblank end -------- Vblank Start -- (HIGH)
  // So int6 happens in the middle of the count

  int coo_waiting; // after 04 write which is 0x4xxx or 0x8xxx
  unsigned int coo_cmd; // The complete command
  int aoo3_toggle,aoo5_toggle,aoo3_six,aoo5_six;
  int aoo3_six_timeout, aoo5_six_timeout;
  int calculate_hcoord();
  unsigned char  calculate_coo8();
  unsigned char  calculate_coo9();
  int may_want_to_get_pic(struct bmap *bm,unsigned char retpal[256],int mark);
  int may_want_to_get_sound(struct sndinfo *sndi);

  void run_to_odo_star(int odo_to);
  void run_to_odo_musa(int odo_to);
  void run_to_odo_m68kem(int odo_to);
  void run_to_odo_z80(int odo_to);

  int fm_timer_callback();
  int myfm_read(int a);
  int mysn_write(int v);
  int fm_sel[2],fm_tover[2],ras_fm_ticker[4];
  signed short fm_reg[2][0x100]; // All of them (-1 = not def'd yet)

  int dac_data[0x138], dac_enabled, dac_last;
  // Since dac values are normally shifted << 6, 1 is used to mark unchanged
  // values.
  void dac_init() { dac_last = 1; dac_enabled = 1; dac_clear(); }
  void dac_clear() {
    dac_data[0] = dac_last;
    for(int i = 1; i < 0x138; ++i)
      dac_data[i] = 1;
  }
  void dac_submit(int d) {
    dac_last = (d - 0x80) << 6; if(dac_enabled) dac_data[ras] = dac_last;
  }
  void dac_enable(int d) {
    dac_enabled = d & 0x80;
    dac_data[ras] = (dac_enabled? dac_last : 1);
  }

public:
  int myfm_write(int a,int v,int md);
  // public struct, full with data from the cartridge header
  struct _carthead_ {
    char system_name[0x10];           // "SEGA GENESIS    ", "SEGA MEGA DRIVE  "
    char copyright[0x10];             // "(C)SEGA 1988.JUL"
    char domestic_name[0x30];         // Domestic game name
    char overseas_name[0x30];         // Overseas game name
    char product_no[0xe];             // "GM XXXXXXXX-XX" or "GM MK-XXXX -00"
    unsigned short checksum;          // ROM checksum
    char control_data[0x10];          // I/O use data (usually only joystick)
    unsigned rom_start, rom_end;      // ROM start & end addresses
    unsigned ram_start, ram_end;      // RAM start & end addresses
    unsigned short save_magic;        // 0x5241("RA") if saveram, else 0x2020
    unsigned short save_flags;        // Misc saveram info
    unsigned save_start, save_end;    // Saveram start & end
    unsigned short modem_magic;       // 0x4d4f("MO") if uses modem, else 0x2020
    char modem_firm[4];               // Modem firm name (should be same as (c))
    char modem_ver[4];                // yy.z, yy=ID number, z=version
    char memo[0x28];                  // Extra data
    char countries[0x10];             // Country code
  } cart_head;
  char region; // Emulator region.
  int pal; // 0 = NTSC 1 = PAL
  int one_frame_star(struct bmap *bm,unsigned char retpal[256],struct sndinfo *sndi);
  int one_frame_musa(struct bmap *bm,unsigned char retpal[256],struct sndinfo *sndi);
  int one_frame_m68kem(struct bmap *bm,unsigned char retpal[256],struct sndinfo *sndi);
  int one_frame(struct bmap *bm,unsigned char retpal[256],struct sndinfo *sndi);

  int pad[2];
// c000004 bit 1 write fifo empty, bit 0 write fifo full (???)
// c000005 vint happened, (sprover, coll, oddinint)
// invblank, inhblank, dma busy, pal
  unsigned char coo4,coo5;
  int okay() {return ok;}
  md(bool pal, char region);
  ~md();
  int plug_in(unsigned char *cart,int len);
  int unplug();
  int load(char *name);

  int reset();

  unsigned misc_readbyte(unsigned a);
  void misc_writebyte(unsigned a,unsigned d);
  unsigned misc_readword(unsigned a);
  void misc_writeword(unsigned a,unsigned d);

  int z80_init();
  unsigned char z80_read(unsigned int a);
  void z80_write(unsigned int a,unsigned char v);
  unsigned short z80_port_read(unsigned short a);
  void z80_port_write(unsigned short a,unsigned char v);

  enum z80_core {
    Z80_CORE_NONE,
#ifdef WITH_MZ80
    Z80_CORE_MZ80,
#endif
#ifdef WITH_CZ80
    Z80_CORE_CZ80,
#endif
    Z80_CORE_TOTAL
  } z80_core;
  void cycle_z80();

  enum cpu_emu {
    CPU_EMU_NONE,
#ifdef WITH_STAR
    CPU_EMU_STAR,
#endif
#ifdef WITH_MUSA
    CPU_EMU_MUSA,
#endif
    CPU_EMU_TOTAL
  } cpu_emu; // OK to read it but call cycle_cpu() to change it
  void cycle_cpu();

#ifdef WITH_MZ80
  mz80context&   z80_context() {return z80;}
#endif
  int load(FILE *hand,char *desc);
  int save(FILE *hand,char *desc);
  int import_gst(FILE *hand);
  int export_gst(FILE *hand);

  char romname[256];

  int z80dump();

  // Fix ROM checksum
  void fix_rom_checksum();

  // List of patches currently applied.
  struct patch_elem {
    struct patch_elem *next;
    uint32_t addr;
    uint16_t data;
  } *patch_elem;
  // Added by Joe Groff:
  // Patch the ROM code, using Game Genie/Hex codes
  int patch(const char *list, unsigned int *errors,
	    unsigned int *applied, unsigned int *reverted);
  // Get/put the battery save RAM
  int has_save_ram();
  int get_save_ram(FILE *from);
  int put_save_ram(FILE *into);

  // Added by Phillip K. Hornung <redx@pknet.com>
  // Linux joystick initialization and handling routines
  void init_joysticks();
  void read_joysticks();
};

inline int md::has_save_ram()
{
  return save_len;
}

#endif // __MD_H__
