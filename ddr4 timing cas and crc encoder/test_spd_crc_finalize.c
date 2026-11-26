#include "spd_crc.h"
#include "spd_crc_finalize.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                      \
    do                                                        \
    {                                                         \
        if (!(condition))                                     \
        {                                                     \
            fprintf(stderr,                                   \
                    "FAIL: %s:%d: %s\n",                      \
                    __FILE__,                                 \
                    __LINE__,                                 \
                    #condition);                              \
            return 1;                                         \
        }                                                     \
    } while (0)

static int test_crc_known_answer(void)
{
    static const uint8_t data[] =
    {
        '1', '2', '3', '4', '5',
        '6', '7', '8', '9'
    };

    uint16_t crc = UINT16_C(0);

    CHECK(
        spd_crc16_ccitt_zero(data, sizeof data, &crc) == SPD_CRC_STATUS_OK); 

    CHECK(crc == UINT16_C(0x31C3));

    return 0;
}

static int test_crc_empty_input(void)
{
    uint16_t crc = UINT16_C(0xFFFF);

    CHECK(
        spd_crc16_ccitt_zero(NULL, 0U, &crc) == SPD_CRC_STATUS_OK); 

    CHECK(crc == UINT16_C(0x0000));

    return 0;
}

static int test_finalize_argument_validation(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    CHECK(
        spd_crc_finalize(NULL) == SPD_CRC_FINALIZE_INVALID_ARGUMENT);

    CHECK(
        spd_crc_finalize(&image) == SPD_CRC_FINALIZE_NOT_INITIALIZED); 

    return 0;
}

static int test_actual_crc_finalization(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    uint16_t expected_base_crc;
    uint16_t expected_block_crc;

    CHECK(
        spd_image_init(&image) ==
        SPD_ENCODER_OK);

    /* Controlled synthetic image.*/
    memset(image.bytes, 0, sizeof image.bytes);

    /* Put known synthetic data in the two CRC-covered regions. */
    image.bytes[0] = UINT8_C(0x23);
    image.bytes[1] = UINT8_C(0x11);
    image.bytes[2] = UINT8_C(0x0C);
    image.bytes[18] = UINT8_C(0x05);
    image.bytes[24] = UINT8_C(0x6E);

    image.bytes[128] = UINT8_C(0xAA);
    image.bytes[129] = UINT8_C(0x55);
    image.bytes[200] = UINT8_C(0x5A);
    image.bytes[253] = UINT8_C(0xA5);

    /* Independently calculate expected CRC values using the already verified generic CRC routine.*/
    CHECK(
        spd_crc16_ccitt_zero(&image.bytes[0], 126U,
            &expected_base_crc) == SPD_CRC_STATUS_OK); 

    CHECK(
        spd_crc16_ccitt_zero(&image.bytes[128], 126U, 
            &expected_block_crc) == SPD_CRC_STATUS_OK);    

    /* Actual finalizer must now succeed. */
    CHECK(
        spd_crc_finalize(&image) == SPD_CRC_FINALIZE_OK);

    /* Base CRC: bytes 0..125 result stored LSB/MSB at bytes 126/127. */
    CHECK(
        image.bytes[126] == (uint8_t)
        (expected_base_crc & UINT16_C(0x00FF)));  

    CHECK(
        image.bytes[127] == (uint8_t)(
            (expected_base_crc >> 8) & UINT16_C(0x00FF))); 

    /* Module-specific CRC: bytes 128..253 result stored LSB/MSB at bytes 254/255. */ 
    CHECK(
        image.bytes[254] == (uint8_t)(
            expected_block_crc & UINT16_C(0x00FF)));

    CHECK(
        image.bytes[255] == (uint8_t)(
            (expected_block_crc >> 8) & UINT16_C(0x00FF))); 

    return 0;
}

static int test_base_crc_corruption(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    uint16_t stored_crc;
    uint16_t calculated_crc;

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0, sizeof image.bytes);

    image.bytes[2] = UINT8_C(0x0C);
    image.bytes[18] = UINT8_C(0x05);
    image.bytes[24] = UINT8_C(0x6E);

    CHECK( 
        spd_crc_finalize(&image) == SPD_CRC_FINALIZE_OK); 

    stored_crc =
        (uint16_t)image.bytes[126] | ((uint16_t)image.bytes[127] << 8); 

    /* Corrupt one byte inside base CRC coverage. */
    image.bytes[24] ^= UINT8_C(0x01);

    CHECK(
        spd_crc16_ccitt_zero(
            &image.bytes[0], 126U,
            &calculated_crc) == SPD_CRC_STATUS_OK); 

    CHECK(calculated_crc != stored_crc);

    return 0;
}

static int test_block_crc_corruption(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    uint16_t stored_crc;
    uint16_t calculated_crc;

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0, sizeof image.bytes);

    image.bytes[128] = UINT8_C(0xAA);
    image.bytes[200] = UINT8_C(0x55);

    CHECK(
        spd_crc_finalize(&image) == SPD_CRC_FINALIZE_OK);

    stored_crc =
        (uint16_t)image.bytes[254] |
        ((uint16_t)image.bytes[255] << 8);

    /* Corrupt one byte in bytes 128..253. */
    image.bytes[200] ^= UINT8_C(0x01);

    CHECK(
        spd_crc16_ccitt_zero(
            &image.bytes[128], 126U,
            &calculated_crc) == SPD_CRC_STATUS_OK); 

    CHECK(calculated_crc != stored_crc);

    return 0;
}

static int test_crc_bytes_not_in_coverage(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    uint16_t before_base_crc;
    uint16_t after_base_crc;

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0, sizeof image.bytes);

    CHECK(
        spd_crc_finalize(&image) == SPD_CRC_FINALIZE_OK); 

    before_base_crc =
        (uint16_t)image.bytes[126] |
        ((uint16_t)image.bytes[127] << 8);

    /* Changing the stored CRC bytes themselves must not affect calculation over bytes 0..125.*/
    image.bytes[126] ^= UINT8_C(0xFF);
    image.bytes[127] ^= UINT8_C(0xFF);

    CHECK(
        spd_crc16_ccitt_zero(
            &image.bytes[0], 126U,
            &after_base_crc) == SPD_CRC_STATUS_OK); 

    CHECK(after_base_crc == before_base_crc);

    return 0;
}

static int test_status_strings(void)
{
    CHECK(
        strcmp(
            spd_crc_finalize_status_string(SPD_CRC_FINALIZE_OK), "OK") == 0); 

    CHECK(
        strcmp(
            spd_crc_finalize_status_string(SPD_CRC_FINALIZE_INVALID_ARGUMENT),
            "INVALID_ARGUMENT") == 0); 

    CHECK(
        strcmp(
            spd_crc_finalize_status_string(
                SPD_CRC_FINALIZE_NOT_INITIALIZED), "NOT_INITIALIZED") == 0); 

    /* This only verifies the status string.c It does not mean the finalizer returns
      MISSING_SPECIFICATION in normal supported operation. */  
    CHECK(
        strcmp(
            spd_crc_finalize_status_string(
                SPD_CRC_FINALIZE_MISSING_SPECIFICATION),
            "MISSING_SPECIFICATION") == 0);

    return 0;
}

int main(void)
{
    CHECK(test_crc_known_answer() == 0);

    printf(
        "CRC-16/XMODEM known-answer test : PASS\n");

    printf(
        "123456789                       : 0x31C3 PASS\n");

    CHECK(test_crc_empty_input() == 0);

    printf(
        "Empty input                     : 0x0000 PASS\n");

    CHECK(test_finalize_argument_validation() == 0);

    printf(
        "CRC finalizer argument checks   : PASS\n");

    CHECK(test_actual_crc_finalization() == 0);

    printf(
        "Base CRC calculation            : PASS\n");

    printf(
        "Base CRC placement 126/127      : PASS\n");

    printf(
        "Block-1 CRC calculation         : PASS\n");

    printf(
        "Block-1 CRC placement 254/255   : PASS\n");

    CHECK(test_base_crc_corruption() == 0);

    printf(
        "Base CRC corruption detection   : PASS\n");

    CHECK(test_block_crc_corruption() == 0);

    printf(
        "Block-1 corruption detection    : PASS\n");

    CHECK(test_crc_bytes_not_in_coverage() == 0);

    printf(
        "CRC coverage boundary handling  : PASS\n");

    CHECK(test_status_strings() == 0);

    printf(
        "CRC status string validation    : PASS\n");

    printf("\n");

    printf(
        "SPD CRC finalization            : PASS\n");

    printf(
        "Implemented CRC tests           : ALL PASS\n");

    return 0;
}   