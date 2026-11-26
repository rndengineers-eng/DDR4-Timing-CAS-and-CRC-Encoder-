#include "spd_timing_encode.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DDR4_MTB_PS 125U 

static int has_text(const char *text) 
{
    if (text == NULL)
    {
        return 0;
    }

    while (*text != '\0')
    {
        if (!isspace((unsigned char)*text))
        {
            return 1;
        }

        ++text;
    }

    return 0;
}

static SpdTimingEncodeStatus encode_mtb_ftb_u8(
    uint64_t ps,
    uint8_t *mtb,
    int8_t *ftb)
{
    uint64_t coarse;
    uint64_t remainder;

    if ((mtb == NULL) ||
        (ftb == NULL) ||
        (ps == UINT64_C(0))) 
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    coarse = ps / (uint64_t)DDR4_MTB_PS;
    remainder = ps % (uint64_t)DDR4_MTB_PS;

    if (coarse > UINT8_MAX)
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    if (remainder > INT8_MAX)
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    *mtb = (uint8_t)coarse;
    *ftb = (int8_t)remainder;

    return SPD_TIMING_ENCODE_OK;
}

static SpdTimingEncodeStatus encode_mtb_u12(
    uint64_t ps,
    uint16_t *value)
{
    uint64_t coarse;

    if ((value == NULL) ||
        (ps == UINT64_C(0)))
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    coarse =
        (ps + (uint64_t)(DDR4_MTB_PS - 1U)) /
        (uint64_t)DDR4_MTB_PS;

    if (coarse > UINT64_C(0x0FFF))
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    *value = (uint16_t)coarse;

    return SPD_TIMING_ENCODE_OK;
}

static SpdTimingEncodeStatus encode_mtb_ftb_u12(
    uint64_t ps,
    uint16_t *mtb,
    int8_t *ftb)
{
    uint64_t coarse;
    uint64_t remainder;

    if ((mtb == NULL) ||
        (ftb == NULL) ||
        (ps == UINT64_C(0)))
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    coarse = ps / (uint64_t)DDR4_MTB_PS;
    remainder = ps % (uint64_t)DDR4_MTB_PS;

    if (coarse > UINT64_C(0x0FFF))
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    if (remainder > INT8_MAX)
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    *mtb = (uint16_t)coarse;
    *ftb = (int8_t)remainder;

    return SPD_TIMING_ENCODE_OK;
}

static SpdTimingEncodeStatus encode_mtb_u16(
    uint64_t ps,
    uint16_t *value)
{
    uint64_t coarse;

    if ((value == NULL) ||
        (ps == UINT64_C(0)))
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    coarse =
        (ps + (uint64_t)(DDR4_MTB_PS - 1U)) /
        (uint64_t)DDR4_MTB_PS;

    if (coarse > UINT16_MAX)
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    *value = (uint16_t)coarse;

    return SPD_TIMING_ENCODE_OK;
}

SpdTimingEncodeStatus spd_timing_inputs_check(
    const SpdTimingInputs *timings)
{
    const SpdTimingInput *fields[13U];
    size_t i;

    if (timings == NULL)
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    fields[0]  = &timings->tck_min;
    fields[1]  = &timings->taa_min;
    fields[2]  = &timings->trcd_min;
    fields[3]  = &timings->trp_min;
    fields[4]  = &timings->tras_min;
    fields[5]  = &timings->trc_min;
    fields[6]  = &timings->trfc1_min;
    fields[7]  = &timings->trfc2_min;
    fields[8]  = &timings->trfc4_min;
    fields[9]  = &timings->tfaw_min;
    fields[10] = &timings->trrd_s_min;
    fields[11] = &timings->trrd_l_min;
    fields[12] = &timings->tccd_l_min;

    for (i = 0U; i < 13U; ++i)
    {
        if (fields[i]->known &&
            (fields[i]->ps == UINT64_C(0)))
        {
            return SPD_TIMING_ENCODE_INVALID_VALUE;
        }
    }

    for (i = 0U; i < 13U; ++i)
    {
        if (!fields[i]->known)
        {
            return SPD_TIMING_ENCODE_INCOMPLETE;
        }
    }

    return SPD_TIMING_ENCODE_OK;
}

SpdTimingEncodeStatus spd_timing_encode(const SpdTimingInputs *timings,
    SpdImage *image) 
{
    SpdTimingEncodeStatus status;

    uint8_t tck_mtb;
    uint8_t taa_mtb;
    uint8_t trcd_mtb;
    uint8_t trp_mtb;
    uint8_t trrd_s_mtb;
    uint8_t trrd_l_mtb;
    uint8_t tccd_l_mtb;

    int8_t tck_ftb;
    int8_t taa_ftb;
    int8_t trcd_ftb;
    int8_t trp_ftb;
    int8_t trc_ftb;
    int8_t trrd_s_ftb;
    int8_t trrd_l_ftb;
    int8_t tccd_l_ftb;

    uint16_t tras;
    uint16_t trc;
    uint16_t trfc1;
    uint16_t trfc2;
    uint16_t trfc4;
    uint16_t tfaw;

    uint8_t temp[SPD_IMAGE_SIZE];

    if ((timings == NULL) || (image == NULL)) 
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    if (!image->initialized)
    {
        return SPD_TIMING_ENCODE_NOT_INITIALIZED;
    }

    status = spd_timing_inputs_check(timings);

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->tck_min.ps,
        &tck_mtb, &tck_ftb);  

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->taa_min.ps,
        &taa_mtb, &taa_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->trcd_min.ps,
        &trcd_mtb, &trcd_ftb); 
 
    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->trp_min.ps,
        &trp_mtb, &trp_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_u12(timings->tras_min.ps,
        &tras); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u12(timings->trc_min.ps,
        &trc, &trc_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_u16(timings->trfc1_min.ps,
        &trfc1); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_u16(timings->trfc2_min.ps,
        &trfc2); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_u16(timings->trfc4_min.ps,
        &trfc4); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_u12(timings->tfaw_min.ps,
        &tfaw); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->trrd_s_min.ps,
        &trrd_s_mtb, &trrd_s_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->trrd_l_min.ps,
        &trrd_l_mtb, &trrd_l_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    status = encode_mtb_ftb_u8(timings->tccd_l_min.ps,
        &tccd_l_mtb, &tccd_l_ftb); 

    if (status != SPD_TIMING_ENCODE_OK)
    {
        return status;
    }

    memcpy(temp, image->bytes, sizeof temp);

    temp[17] = UINT8_C(0x00);

    temp[18] = tck_mtb;
    temp[125] = (uint8_t)tck_ftb;

    temp[24] = taa_mtb;
    temp[123] = (uint8_t)taa_ftb; 

    temp[25] = trcd_mtb;
    temp[122] = (uint8_t)trcd_ftb;

    temp[26] = trp_mtb;
    temp[121] = (uint8_t)trp_ftb; 

    temp[27] = (uint8_t) ((((trc >> 8) & UINT16_C(0x000F)) << 4) |
            ((tras >> 8) & UINT16_C(0x000F))); 

    temp[28] =
        (uint8_t)(tras & UINT16_C(0x00FF));
 
    temp[29] =
        (uint8_t)(trc & UINT16_C(0x00FF));

    temp[120] = (uint8_t)trc_ftb;

    temp[30] =
        (uint8_t)(trfc1 & UINT16_C(0x00FF));

    temp[31] =
        (uint8_t)((trfc1 >> 8) & UINT16_C(0x00FF));

    temp[32] =
        (uint8_t)(trfc2 & UINT16_C(0x00FF));

    temp[33] =
        (uint8_t)((trfc2 >> 8) & UINT16_C(0x00FF)); 

    temp[34] =
        (uint8_t)(trfc4 & UINT16_C(0x00FF));

    temp[35] =
        (uint8_t)((trfc4 >> 8) & UINT16_C(0x00FF)); 

    /*
     * tFAW
     * Byte 36 lower nibble = tFAW upper bits.
     */
    temp[36] = (uint8_t) ((temp[36] & UINT8_C(0xF0)) | ((tfaw >> 8) &
             UINT16_C(0x000F))); 

    temp[37] =
        (uint8_t)(tfaw & UINT16_C(0x00FF));

    temp[38] = trrd_s_mtb;
    temp[119] = (uint8_t)trrd_s_ftb;

    temp[39] = trrd_l_mtb;
    temp[118] = (uint8_t)trrd_l_ftb;

    temp[40] = tccd_l_mtb;
    temp[117] = (uint8_t)tccd_l_ftb;

    memcpy(image->bytes, temp, sizeof temp);

    return SPD_TIMING_ENCODE_OK;
}

static SpdTimingEncodeStatus cas_set(
    uint8_t bitmap[4U],
    unsigned cl)
{
    unsigned index;
    unsigned byte_index;
    unsigned bit_index;

    if (bitmap == NULL)
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    /* Supported basic DDR4 CAS range: CL7 through CL36.*/
    if ((cl < 7U) ||
        (cl > 36U))
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    index = cl - 7U;

    byte_index = index / 8U;
    bit_index = index % 8U;

    if (byte_index >= 4U)
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    /* Byte 23 uses bits 0..5 for CL31..CL36.*/
    if ((byte_index == 3U) &&
        (bit_index > 5U))
    {
        return SPD_TIMING_ENCODE_INVALID_VALUE;
    }

    bitmap[byte_index] |= (uint8_t)(
            UINT8_C(1) << bit_index); 

    return SPD_TIMING_ENCODE_OK;
}

SpdTimingEncodeStatus spd_cas_encode(const char *supported_cas_latencies,
    SpdImage *image) 
{
    uint8_t bitmap[4U] =
    {
        UINT8_C(0),
        UINT8_C(0),
        UINT8_C(0),
        UINT8_C(0)
    };

    const char *p;
    unsigned count = 0U;

    if ((supported_cas_latencies == NULL) ||
        (image == NULL))
    {
        return SPD_TIMING_ENCODE_INVALID_ARGUMENT;
    }

    if (!image->initialized)
    {
        return SPD_TIMING_ENCODE_NOT_INITIALIZED;
    }

    if (!has_text(supported_cas_latencies))
    {
        return SPD_TIMING_ENCODE_INCOMPLETE;
    }

    p = supported_cas_latencies;

    while (*p != '\0')
    {
        unsigned value = 0U;
        int have_digit = 0;
        SpdTimingEncodeStatus status;

        /* supported separators.*/
        while ((*p != '\0') &&
               (isspace((unsigned char)*p) ||
                (*p == ',') ||
                (*p == ';')))
        {
            ++p;
        }

        if (*p == '\0')
        {
            break;
        }

        /* Optional CL prefix.*/
        if (((p[0] == 'C') ||
             (p[0] == 'c')) &&
            ((p[1] == 'L') ||
             (p[1] == 'l')))
        {
            p += 2;
        }

        while (isdigit((unsigned char)*p))
        {
            have_digit = 1;

            value = (value * 10U) + (unsigned)(*p - '0');
            ++p;
        } 

        if (!have_digit)
        {
            return SPD_TIMING_ENCODE_INVALID_VALUE;
        }

        status = cas_set(bitmap, value);

        if (status != SPD_TIMING_ENCODE_OK)
        {
            return status;
        }

        ++count;

        /* Require a valid separator or another CAS token. */
        if ((*p != '\0') &&
            !isspace((unsigned char)*p) &&
            (*p != ',') &&
            (*p != ';') &&
            (*p != 'C') &&
            (*p != 'c') &&
            !isdigit((unsigned char)*p))
        {
            return SPD_TIMING_ENCODE_INVALID_VALUE;
        }
    }

    if (count == 0U)
    {
        return SPD_TIMING_ENCODE_INCOMPLETE;
    }

    /* CAS bitmap.*/
    image->bytes[20] = bitmap[0];
    image->bytes[21] = bitmap[1];
    image->bytes[22] = bitmap[2];

    /* Preserve upper reserved bits of byte 23.*/
    image->bytes[23] =
        (uint8_t)(
            (image->bytes[23] & UINT8_C(0xC0)) |
            (bitmap[3] & UINT8_C(0x3F)));

    return SPD_TIMING_ENCODE_OK;
}

const char *spd_timing_encode_status_string(
    SpdTimingEncodeStatus status)
{
    switch (status)
    {
        case SPD_TIMING_ENCODE_OK:
            return "OK";

        case SPD_TIMING_ENCODE_INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";

        case SPD_TIMING_ENCODE_NOT_INITIALIZED:
            return "NOT_INITIALIZED";

        case SPD_TIMING_ENCODE_INCOMPLETE:
            return "INCOMPLETE";

        case SPD_TIMING_ENCODE_INVALID_VALUE:
            return "INVALID_VALUE";

        case SPD_TIMING_ENCODE_MISSING_SPECIFICATION:
            return "MISSING_SPECIFICATION";

        default:
            return "UNKNOWN_STATUS";
    }
}   