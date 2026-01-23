#include "ide.h"

#include "ID/Space.c"


#include <sys/i386/ppa.h>
#include <sys/conf.h>

#include <sys/i386/xhd.h>

#include <sys/mdisk.h>
#undef drive
#include <sys/minidisk.h>
#undef drive


int 	ataopen(dev_t *, int, int, cred_t *);
int 	ataclose(dev_t, int, int, cred_t *);
int 	ataioctl(dev_t, int, caddr_t, int, cred_t *, void *);
int 	ataread(dev_t, struct uio *, cred_t *);
int 	atawrite(dev_t, struct uio *, cred_t *);

void wrapped_ataopen(dev_t dev, int flag, caddr_t ext) {
    dbg_hilvl("ata: wrapped_ataopen dev=0x%x flag=0x%x\n", dev, flag);

    /*
    To slam on full debug output to look the interaction with a particular device,
    here you could do something like:

    if ((dev == 0x300a0) && (flag == 2)) {
        // open debug
        atadebug = 9;
    }
    */

    // make sure the partition and vtoc info is loaded and our xhd link is initialized
	ata_partinit();

    /* In the case of autoconf from xhd we will get an open() with xhd drive 0 (potentially invalid).
    /  This call is just meant as signal to initialize the xhd. */
    if (flag & FAUTOCONF) {
        return;
    }

	int ret = ataopen(&dev, flag, (int)ext, NULL);

	if (ret) {
        dbg_hilvl("ata: wrapper: ataopen returned %d\n", ret);
		u.u_error = ret;
	}
}

void wrapped_ataclose(dev_t dev, int flag, caddr_t ext) {
    dbg_hilvl("ata: wrapped_ataclose dev=0x%x flag=0x%x\n", dev, flag);
	int ret = ataclose(dev, flag, (int)ext, NULL);
	if (ret) {
		u.u_error = ret;
	}
}

void wrapped_ataioctl(dev_t dev, u_long cmd, caddr_t data, int flag, void *ext) {
    dbg_hilvl("ata: wrapped_ataioctl cmd=0x%x major(%d) minor(%d)\n", cmd, major(dev), minor(dev));
	int ret = ataioctl(dev, cmd, data, flag, NULL, ext);

	if (ret) {
        dbg_hilvl("ata: ioctl ret %d\n", ret);
		u.u_error = ret;
	}
}

int atadump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
     printf("ata: atadump() no-op\n");
	return ENODEV;
}

struct iobuf ata_iobuf;

struct xhd_link ata_xhd_link;
int xhd_main_major;
int xhd_partitions_major;

/* xhd interface */
extern int xhd_register_dd(struct xhd_link * xhl);
extern int xhdmajor[2];
extern int hdinitpart(dev_t xhd_dev, dev_t mxhd_dev, caddr_t read_buf, struct hdpart * hdpart_array, int vtocflg[2][2]);

int atahdinit_previously_called = 0;

void
atahdpark(dev_t devno) {
    printf("ata: atahdpark(dev=0x%x)\n", devno);

    ATADEBUG(3, "park flushing ata\n");
    for (int i = 0 ; i < ATA_MAX_CTRL; i++) {
        if (AC_HAS_FLAG(&ata_ctrl[i],ACF_PRESENT)) {
            for (int drive = 0; drive < ATA_MAX_DRIVES; drive++) {
                ata_unit_t * u = ata_ctrl[i].drive[drive];
                if (u != NULL) {
                    if (U_HAS_FLAG(u,UF_PRESENT)) {

                        // don't need to flush a cdrom.
                        if (U_HAS_FLAG(u,UF_CDROM))
                            continue;

                        ata_ctrl_t *ac = &ata_ctrl[i];
                        ata_flush_cache(ac,drive);

                    }
                }
            }
        }
    }

    ATADEBUG(3, "park done\n");
}

struct hdpart atahd_hdparts[ATA_MAX_CTRL * ATA_MAX_DRIVES][MAX_PARTS];

struct hdpart *
atahd_get_partition(int drive_num, int slice) {
    //printf("ata: atahd_get_partition(%d, %d)\n", drive_num, slice);
    return &atahd_hdparts[drive_num][slice];
}

#define WHOLE_DRIVE_SLICE 0
#define PARITTIONS_SLICE 32

int dev_to_controller_drive(dev_t dev) {
    int out = (minor(dev) >> 5) - ata_xhd_link.drive_offset;
    //printf("ata: dev_to_controller_drive(dev=0x%x) -> drive %d\n", dev, out);
    return out;
}

int dev_to_slice(dev_t dev) {
    return minor(dev) & 0x1f;
}

/* for now let's fit our whole drive number space without trying to remap actually present drives */
dev_t to_xhd_drive_dev(int ctrl, int drive, int slice) {
    int our_drive_num = ctrl * ATA_MAX_DRIVES + drive + ata_xhd_link.drive_offset;
    int out;
    if (slice < PARITTIONS_SLICE) {
        out = xhd_main_major << 16 | our_drive_num << 5 | slice;
    } else {
        out = xhd_partitions_major << 16 | our_drive_num << 5 | (slice-PARITTIONS_SLICE);
    }
    return out;
}

int is_mbr_part(dev_t dev) {
    return major(dev) == xhd_partitions_major;
}

struct partition * partition_from_dev(dev_t dev) {
    //printf("ata: partition_from_dev(0x%x)\n", dev);
    int drive, slice;
    drive = (minor(dev) >> 5) - ata_xhd_link.drive_offset;

    if (drive < 0) return NULL;
    if (drive >= ATA_MAX_CTRL * ATA_MAX_DRIVES) return NULL;

    if (major(dev) == xhd_main_major) {
        slice = minor(dev) & 0x1f;
    } else if (major(dev) == xhd_partitions_major) {
        slice = (minor(dev) & 0x1f) + 32;
    } else {
        return NULL;
    }

    if (slice < 0) return NULL;
    if (slice >= 37) return NULL;

    /* struct partition in the AIX case in ide.h is just our struct hdparts with different field names */
    return (struct partition *) &atahd_hdparts[drive][slice];
}

void ata_summarize_drives();

int ata_reload_mbrs_and_vtocs() {
    caddr_t read_buf = (caddr_t)kmem_alloc(512 * 7, KM_SLEEP);
    if (!read_buf) {
        printf("ata: no mem while allocating hdpart buf");
        return EFAULT;
    }

    for (int i = 0 ; i < ATA_MAX_CTRL; i++) {
        if (AC_HAS_FLAG(&ata_ctrl[i],ACF_PRESENT)) {
            for (int drive = 0; drive < ATA_MAX_DRIVES; drive++) {
                ata_unit_t * u = ata_ctrl[i].drive[drive];
                if (u != NULL) {
                    if (U_HAS_FLAG(u,UF_PRESENT)) {

                        // don't try to read a partition table from a cdrom.
                        if (U_HAS_FLAG(u,UF_CDROM))
                            continue;

                        int vtocflg[2][2];
                        int hdinitpart_result = hdinitpart(to_xhd_drive_dev(i, drive, WHOLE_DRIVE_SLICE), to_xhd_drive_dev(i, drive, PARITTIONS_SLICE), read_buf, atahd_hdparts[i * ATA_MAX_DRIVES + drive], vtocflg);
                        if (hdinitpart_result != 0) {
                            //printf("ata: hdinitpart() reading vtoc errored on controller %d drive %d\n", i, drive);
                        }

                        u->vtoc_valid = hdinitpart_result == 0;
                    }
                }
            }
        }
    }

    kmem_free(read_buf, 512 * 7);

    return 0;
}

int ata_partinit_already = 0;

int ata_partinit() {

    if (ata_partinit_already)
        return 0;
    ata_partinit_already = 1;

    printf("ata: ata_partinit()\n");

    /* if we do some kind of more sophisticated packing of our drive numbers into the xhd number space
    we need to produce an accurate size of our space here */
    ata_xhd_link.cur_drives = ATA_MAX_CTRL * ATA_MAX_DRIVES;
    ata_xhd_link.flags |= XHDL_DRIVES_VALID;

    // TODO populate 0 with ata structs geometry data or nah?

    int load_result = ata_reload_mbrs_and_vtocs();
    if (load_result != 0)
        return load_result;

    ata_summarize_drives();

    return 0;
}

void
ata_summarize_drives() {
    printf("ata: Summary of drives:\n");
    for (int i = 0 ; i < ATA_MAX_CTRL; i++) {

        /* go through the drives in this controller and note any units that have been found */

        if (AC_HAS_FLAG(&ata_ctrl[i],ACF_PRESENT)) {
            printf("ata: controller %d - io_base=0x%x irq=%d\n", i, ata_ctrl[i].io_base, ata_ctrl[i].irq);
            for (int drive = 0; drive < ATA_MAX_DRIVES; drive++) {
                ata_unit_t * u = ata_ctrl[i].drive[drive];
                if (u == NULL) {
                    printf("ata:   no u\n");
                } else {
                    dev_t drive_dev = to_xhd_drive_dev(i, drive, 0);
                    if (U_HAS_FLAG(u,UF_PRESENT)) {
                        printf("ata:   drive %d major %d minor %d\n", drive, major(drive_dev), minor(drive_dev));

                        // ata_summarize_drives is now called by ata_partinit after loading the hdparts
                        // TODO use them to show partitions that are present
                    }
                }
            }
        }
    }
}

void
ata_mbstrategy_trivial(struct buf * flist) {
    int s = splbio();
    while (flist != NULL) {
        struct buf * next = flist->av_forw;
        flist->av_forw = NULL;
        splx(s);
        atastrategy(flist);
        s = splbio();
        flist = next;
    }
    splx(s);
}

void
atahdinit(dev_t devno) {
    printf("ata: svr4 ata/atapi driver ported to aix, \n");
	printf("ata: major %d minor %d\n", major(devno), minor(devno));

    if (atahdinit_previously_called == 0) {
        atahdinit_previously_called = 1;

        /* Tweak the Space settings for our purposes */

        // Debug
        //int	atadebug   = 9;

        // Controllers to look for:
        // enable primary
        ata_ctrl[0].flags = ACF_PRESENT;
        // disable tertiary
        ata_ctrl[2].flags = ACF_NONE;

        ata_xhd_link.next = NULL;
        ata_xhd_link.drive_offset = 0;
        ata_xhd_link.driver_name = "atahd";
        ata_xhd_link.min_drives = 0;
        ata_xhd_link.max_drives = ATA_MAX_CTRL * ATA_MAX_DRIVES;
        ata_xhd_link.flags = XHDL_INITIALIZED | XHDL_PRESENT;
        ata_xhd_link.init = (int(*)())atahdinit;
        ata_xhd_link.open = (int(*)())wrapped_ataopen;
        ata_xhd_link.close = (int(*)())wrapped_ataclose;
        ata_xhd_link.read = ataread;
        ata_xhd_link.write = atawrite;
        ata_xhd_link.ioctl = (int(*)())wrapped_ataioctl;
        ata_xhd_link.park = (int(*)())atahdpark;
        ata_xhd_link.dump = atadump;
        ata_xhd_link.strategy = atastrategy;
        ata_xhd_link.mbstrategy = (int(*)())ata_mbstrategy_trivial;
        ata_xhd_link.get_partition = (int(*)()) atahd_get_partition;

        xhd_register_dd(&ata_xhd_link);
        //printf("ata: xhd_register_dd return\n");
    }

    xhd_main_major = xhdmajor[0];
    xhd_partitions_major = xhdmajor[1];

    /* attaches will been taken care of by atainit() */
    if (atainit()) {
        printf("ata: atainit err\n");
        return;
    }
    //printf("ata: atainit done\n");

}

