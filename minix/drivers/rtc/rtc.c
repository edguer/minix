#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/chardriver.h>

#include "rtc.h"

// Prototypes and dumb implementations
int rtc_open(devminor_t minor, int access, endpoint_t user_endpt) { return OK; }
int rtc_close(devminor_t minor) { return OK; }
int rtc_read(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);

// Entrypoints
struct chardriver rtc_tab =
{
    .cdr_open = rtc_open,
    .cdr_close = rtc_close,
    .cdr_read = rtc_read,
};

int sef_cb_init_fresh(int type, sef_init_info_t *info) { return OK; }

void sef_local_startup()
{
    /* Register init callbacks. */
    sef_setcb_init_fresh(sef_cb_init_fresh);
    sef_setcb_init_lu(sef_cb_init_fresh);      /* treat live updates as fresh inits */
    sef_setcb_init_restart(sef_cb_init_fresh); /* treat restarts as fresh inits */
 
    /* Register live update callbacks. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);         /* agree to update immediately when a LU request is received in a supported state */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard); /* support live update starting from any standard state */
 
    /* Let SEF perform startup. */
    sef_startup();
}

unsigned bcd_to_bin(unsigned value)
{
    return (value & 0x0f) + ((value >> 4) * 10);
}

int cmos_read_byte(int offset)
{
    uint32_t value = 0;
    int r;

    // Although sys_outb and sys_inb send and receive bytes internally, the methods arguments are dword
    if ((r = sys_outb(CMOS_CMD_PORT, offset)) != OK)
    {
        panic("cmos_read_byte.sys_outb failed: %d\n", r);
    }
    if ((r = sys_inb(CMOS_DATA_PORT, &value) != OK))
    {
        panic("cmos_read_byte.sys_inb failed: %d\n", r);
    }

    return value;
}

// Checking if Status B most relevant bit is set
unsigned char cmos_update_inprogress(void)
{
    return !(cmos_read_byte(RTC_STATUS_A) & RTC_UIP);
}

unsigned char cmos_is_bdc(void)
{
    return cmos_read_byte(RTC_STATUS_B) & RTC_BCD;
}

void get_time(char *buffer, int size)
{
    int sec, min, hour, day, mon, year;

    printf("get_time start\n");

    // Busy wait if an update is in progress
    while (cmos_update_inprogress());

    /*
     * Ideally we would need to run this twice to guarantee data accuracy
     * For more info, read https://wiki.osdev.org/CMOS
     * For simplicity though, we are just doing it won't
     */

    // Getting the time
    sec = cmos_read_byte(RTC_SECONDS), min = cmos_read_byte(RTC_MINUTES), hour = cmos_read_byte(RTC_HOURS);
    
    // Getting the date
    day = cmos_read_byte(RTC_DAY_OF_MONTH), mon = cmos_read_byte(RTC_MONTH), year = cmos_read_byte(RTC_YEAR);

    // If in BDC format, do the math
    if (cmos_is_bdc())
    {
        printf("get_time in bdc format\n");
        
        // Adjust the time
        sec = bcd_to_bin(sec), min = bcd_to_bin(min), hour = bcd_to_bin(hour);
        
        // Adjust the date
        day = bcd_to_bin(day), mon = bcd_to_bin(mon), year = bcd_to_bin(year);
    }

    // Format and write to buffer
    snprintf(buffer, size, "%04d-%02x-%02x %02x:%02x:%02x\n", year + 2000, mon, day, hour, min, sec);
}

ssize_t rtc_read(devminor_t minor, u64_t position, endpoint_t endpt, cp_grant_id_t grant, size_t size, int flags, cdev_id_t id)
{
    char buffer[1024];
    int bytes, ret, buffer_size = sizeof(buffer);

    printf("rtc_read start\n");

    get_time(buffer, buffer_size);

    bytes = MIN(size - (int) position, buffer_size);

    if (bytes <= 0) return OK;

    ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) buffer + position, bytes);

    printf("rtc_read finish\n");

    return bytes;
}

int main(int argc, char **argv)
{
    sef_local_startup();
 
    chardriver_task(&rtc_tab);
    return OK;
}