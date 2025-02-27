#include <minix/drivers.h>
#include <minix/chardriver.h>

#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>

// Prototypes
static int hello_open(devminor_t minor, int access, endpoint_t user_endpt);
static int hello_close(devminor_t minor);
static ssize_t hello_read(
    devminor_t minor,
    u64_t position,
    endpoint_t endpt,
    cp_grant_id_t grant,
    size_t size,
    int flags,
    cdev_id_t id);

// SEF functions and variables.
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

// Register entrypoints
static struct chardriver hello_tab =
{
    .cdr_open  = hello_open,
    .cdr_close = hello_close,
    .cdr_read  = hello_read,
};

static int open_counter;

static int hello_open(devminor_t UNUSED(minor), int UNUSED(access), endpoint_t UNUSED(user_endpt))
{
    printf("hello open\n");
    return OK;
}
static int hello_close(devminor_t UNUSED(minor))
{
    printf("hello close\n");
    return OK; 
}

static ssize_t hello_read(
    devminor_t UNUSED(minor),
    u64_t position,
    endpoint_t endpt,
    cp_grant_id_t grant,
    size_t size,
    int UNUSED(flags),
    cdev_id_t UNUSED(id))
{
    char *ptr;
    int ret;
    char *buf = "My hello message\n";
    u64_t dev_size = (u64_t) strlen(buf);

    printf("hello read\n");

    if (position >= dev_size) return 0; // There is nothing else to return
    if (position + size > dev_size) size = (size_t)(dev_size - position); // We are at the at, so just return until the end

    ptr = buf + (size_t) position;
    if ((ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) ptr, size)) != OK)
        return ret;

    return size;
}

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
            printf("%s", "My hello message\n");
        break;
 
        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;
 
            printf("%sHey, I'm a new version!\n", "My hello message\n");
        break;
 
        case SEF_INIT_RESTART:
            printf("%sHey, I've just been restarted!\n", "My hello message\n");
        break;
    }
 
    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }
 
    /* Initialization completed successfully. */
    return OK;
}

int main(int argc, char **argv)
{
    sef_local_startup();
 
    chardriver_task(&hello_tab);
    return OK;
}