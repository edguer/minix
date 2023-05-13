#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/chardriver.h>
#include <minix/drivers.h>
#include <termios.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/video.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/sys_config.h>
#include <minix/vm.h>

static int open_counter;

static phys_bytes vid_size;
static phys_bytes vid_base;

static char *vid_mem = NULL;

static u16_t bios_columns, bios_crtbase, bios_fontlines;
static u8_t bios_rows;

// Prototypes and dumb implementations
int vid_open(devminor_t minor, int access, endpoint_t user_endpt);
int vid_close(devminor_t minor) { return OK; }
int vid_read(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
int vid_write(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);

static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

// Entrypoints
struct chardriver vid_tab =
{
    .cdr_open = vid_open,
    .cdr_close = vid_close,
    .cdr_read = vid_read,
    .cdr_write = vid_write,
};

static int sef_cb_lu_state_save(int UNUSED(state)) {
/* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);
 
    return OK;
}
 
static int lu_state_restore() {
/* Restore the state. */
    u32_t value;
 
    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;
 
    return OK;
}
 
static void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);
 
    /*
     * Register live update callbacks.
     */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);
 
    /* Let SEF perform startup. */
    sef_startup();
}
 
static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;
 
    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            printf("vid starting fresh\n");
        break;
 
        case SEF_INIT_LU:
            lu_state_restore();
            do_announce_driver = FALSE;
 
            printf("vid starting\n");
        break;
 
        case SEF_INIT_RESTART:
            printf("vid restarting\n");
        break;
    }
 
    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }
 
    /* Initialization completed successfully. */
    return OK;
}

int vid_open(devminor_t minor, int access, endpoint_t user_endpt)
{
    int s;

    printf("vid_open start\n");

  	s=sys_readbios(VDU_SCREEN_COLS_ADDR, &bios_columns,
		VDU_SCREEN_COLS_SIZE);
  	s=sys_readbios(VDU_CRT_BASE_ADDR, &bios_crtbase,
		VDU_CRT_BASE_SIZE);
  	s=sys_readbios(VDU_SCREEN_ROWS_ADDR, &bios_rows,
		VDU_SCREEN_ROWS_SIZE);
  	s=sys_readbios(VDU_FONTLINES_ADDR, &bios_fontlines,
		VDU_FONTLINES_SIZE);

    printf("vid_open retrieved bios info, result is %d\n", s);

    if (s != OK) return s;

    vid_base = (phys_bytes) COLOR_BASE;
    vid_size = COLOR_SIZE;

    printf("vid_open request permission\n");
    struct minix_mem_range mr;
    mr.mr_base = vid_base;
    mr.mr_limit = vid_base + vid_size;

    if( OK != (s = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
        printf("vid_open sys_privctl (ADD_MEM) failed: %d\n", s);

    printf("vid_open creating memory map");

	vid_mem = vm_map_phys(SELF, (void *) vid_base, vid_size);

	if(vid_mem == MAP_FAILED) 
  		printf("vid_open couldn't map video memory %p\n", vid_mem);

    printf("vid_open finish\n");

    return OK;
}

ssize_t vid_read(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id)
{
    printf("vid_read start\n");

    return 0;
}

ssize_t vid_write(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id)
{
    char buffer[128];
    char *buf_ptr = buffer;
    int r;

    // It fails today, but data is being copied to buffer
    if ((r = sys_safecopyfrom(endpt, grant, position, (vir_bytes)buf_ptr, size)) != OK)
        printf("vid_write error copying incoming data to local buffer %d\n", r);

    printf("vid_write received data: %s\n", buffer);

    // We are writing each character to the start position of each line, so we can see the results in the screen
    for (int i = 0; i < size; i++)
    {
        *(vid_mem + (bios_columns * i)) = *buf_ptr++;
    }

    return 0;
}

int main(int argc, char **argv)
{
    sef_local_startup();
    chardriver_task(&vid_tab);
    return OK;
}