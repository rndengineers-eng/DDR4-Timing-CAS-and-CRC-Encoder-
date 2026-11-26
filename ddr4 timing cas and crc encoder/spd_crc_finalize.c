#include "spd_crc_finalize.h"
#include "spd_crc.h"

#include <stdint.h>

SpdCrcFinalizeStatus spd_crc_finalize(
    SpdImage *image)
{
    uint16_t base_crc;
    uint16_t block1_crc;
    SpdCrcStatus crc_status;

    if (image == NULL)
    {
        return SPD_CRC_FINALIZE_INVALID_ARGUMENT;
    }

    if (!image->initialized)
    {
        return SPD_CRC_FINALIZE_NOT_INITIALIZED;
    }

    crc_status = spd_crc16_ccitt_zero(
        &image->bytes[0], 126U,&base_crc);  

    if (crc_status != SPD_CRC_STATUS_OK)
    {
        return SPD_CRC_FINALIZE_INVALID_ARGUMENT;
    } 

    crc_status = spd_crc16_ccitt_zero(&image->bytes[128], 126U, &block1_crc); 

    if (crc_status != SPD_CRC_STATUS_OK)
    {
        return SPD_CRC_FINALIZE_INVALID_ARGUMENT;
    }

    /* CRC values only after both calculations succeed.*/

    image->bytes[126] = (uint8_t) (base_crc & UINT16_C(0x00FF)); 

    image->bytes[127] = (uint8_t) ((base_crc >> 8) & UINT16_C(0x00FF));

    image->bytes[254] = (uint8_t) (block1_crc & UINT16_C(0x00FF)); 

    image->bytes[255] = (uint8_t) ((block1_crc >> 8) & UINT16_C(0x00FF)); 

    return SPD_CRC_FINALIZE_OK;
}

const char *spd_crc_finalize_status_string(
    SpdCrcFinalizeStatus status)
{
    switch (status)
    {
        case SPD_CRC_FINALIZE_OK:
            return "OK";

        case SPD_CRC_FINALIZE_INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";

        case SPD_CRC_FINALIZE_NOT_INITIALIZED:
            return "NOT_INITIALIZED";

        case SPD_CRC_FINALIZE_MISSING_SPECIFICATION:
            return "MISSING_SPECIFICATION";

        default:
            return "UNKNOWN_STATUS";
    }
}   