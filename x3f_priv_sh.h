#ifndef __INCLUDE_X3F_PRIV_SH_H__
#define __INCLUDE_X3F_PRIV_SH_H__

#include <stdint.h>

#define X3F_MAGIC   'bVOF'

struct x3f_version_id {
    uint16_t major;
    uint16_t minor;
};

#define X3F_DIR_SEC 'dCES'
#define X3F_PROP_LIST_SEC 'pCES'
#define X3F_IMAGE_SEC       'iCES'
#define X3F_IMAGE2_SEC      'iCES'

#define X3F_DIR_IMAG        'GAMI'
#define X3F_DIR_IMA2        '2AMI'
#define X3F_DIR_PROP        'PORP'
#define X3F_DIR_CAMF        'FMAC'


#endif /* __INCLUDE_X3F_PRIV_SH_H__ */

