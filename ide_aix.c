
#include "ide.h"

#ifdef _AIX

#include "ID/Space.c"

#include <sys/i386/ppa.h>
#include <sys/conf.h>

int hdanotherinit() {
    printf("ata: here is hdanotherinit being called\n");
    return 0;
}


int 	ataopen(dev_t *, int, int, cred_t *);
int 	ataclose(dev_t, int, int, cred_t *);
int 	ataioctl(dev_t, int, caddr_t, int, cred_t *, void *);
int 	ataread(dev_t, struct uio *, cred_t *);
int 	atawrite(dev_t, struct uio *, cred_t *);

void wrapped_ataopen(dev_t dev, int flag, caddr_t ext) {
    // printf("ata: wrapped_ataopen\n");
	int ret = ataopen(&dev, flag, (int)ext, NULL);
	if (ret) {
		u.u_error = ret;
	}
}

void wrapped_ataclose(dev_t dev, int flag, caddr_t ext) {
    // printf("ata: wrapped_ataclose\n");
	int ret = ataclose(dev, flag, (int)ext, NULL);
	if (ret) {
		u.u_error = ret;
	}
}

void wrapped_ataioctl(dev_t dev, u_long cmd, caddr_t data, int flag, void *ext) {
    printf("ata: wrapped_ataioctl cmd=0x%x major(%d) minor(%d)\n", cmd, major(dev), minor(dev));
	int ret = ataioctl(dev, cmd, data, flag, NULL, ext);
	if (ret) {
		u.u_error = ret;
	}
}

int atadump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	return ENODEV;
}

void
ata_register_dev(dev_t devno, struct iobuf * iobuf) {
    printf("ata: register major %d iobuf at 0x%x\n", major(devno), iobuf);
    /* See example at AIX PS/2 and System/370 Technical Reference Mar 1991 p. C.4.1.1 - 1 */

    /* their reference docs imply that ddopen, ddclose, ddioctl should not have direct returns */
    DEV_INSTALL(major(devno), hdanotherinit, /*reset*/ nulldev, (int(*)())&wrapped_ataopen, (int(*)())wrapped_ataclose, /*intr*/ nulldev, ISNOTATTY | DV_AUTOCONF);
    iobuf->ib_dev = devno;
    BDEV_INSTALL(major(devno), (int(*)())atastrategy, atadump, iobuf);
    CDEV_INSTALL(major(devno), ataread, atawrite, (int(*)())wrapped_ataioctl, /*select*/ nulldev, notty);
    printf("ata: registration complete\n");
}

struct iobuf ata_iobuf;

void
atahdinit(dev_t devno) {
    printf("ata: svr4 ata/atapi driver ported to aix, \n");
	printf("ata: major %d minor %d\n", major(devno), minor(devno));

    if (atainit()) {
        printf("ata: atainit err\n");
        return;
    }

    for (int i = 0 ; i < ATA_MAX_CTRL; i++) {
        /* skip drives that ata doesn't have as present */
        if (!AC_HAS_FLAG(&ata_ctrl[i],ACF_PRESENT)) {
            printf("ata: %d not present\n", i);
        } else {
            printf("ata: %d present\n", i);
        }

        /* attaches are taken care of by atainit() */

        /* go through the drives in this controller and note any units that have been found */

        if (AC_HAS_FLAG(&ata_ctrl[i],ACF_PRESENT)) {
            printf("ata: controller %d\n", i);
            for (int drive = 0; drive < ATA_MAX_DRIVES; drive++) {
                ata_unit_t * u = ata_ctrl[i].drive[drive];
                if (u == NULL) {
                    printf("ata:   no u\n");
                } else {
                    dev_t drive_dev = devno | ATA_DEV(i, drive);
                    if (U_HAS_FLAG(u,UF_PRESENT)) {
                        printf("ata:   drive %d major %d minor %d\n", drive, major(drive_dev), minor(drive_dev));
                    }
                }
            }
        }
    }

    ata_register_dev(devno, &ata_iobuf);
}

#endif