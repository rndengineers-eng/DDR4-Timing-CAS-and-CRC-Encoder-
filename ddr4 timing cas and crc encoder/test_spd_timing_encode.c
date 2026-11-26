#include "spd_timing_encode.h"

#include <stdbool.h>
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

static SpdTimingInputs make_valid_timings(void)
{
    SpdTimingInputs timings;

    memset(&timings, 0, sizeof timings);

    timings.tck_min =
        (SpdTimingInput){ true, UINT64_C(625) };

    timings.taa_min =
        (SpdTimingInput){ true, UINT64_C(13750) };

    timings.trcd_min =
        (SpdTimingInput){ true, UINT64_C(13750) };

    timings.trp_min =
        (SpdTimingInput){ true, UINT64_C(13750) };

    timings.tras_min =
        (SpdTimingInput){ true, UINT64_C(32000) };

    timings.trc_min =
        (SpdTimingInput){ true, UINT64_C(45750) };

    timings.trfc1_min =
        (SpdTimingInput){ true, UINT64_C(350000) };

    timings.trfc2_min =
        (SpdTimingInput){ true, UINT64_C(260000) };

    timings.trfc4_min =
        (SpdTimingInput){ true, UINT64_C(160000) };

    timings.tfaw_min =
        (SpdTimingInput){ true, UINT64_C(30000) };

    timings.trrd_s_min =
        (SpdTimingInput){ true, UINT64_C(5000) };

    timings.trrd_l_min =
        (SpdTimingInput){ true, UINT64_C(7500) };

    timings.tccd_l_min =
        (SpdTimingInput){ true, UINT64_C(6250) };

    timings.supported_cas_latencies =
        "CL20 CL22";

    return timings;
}

static int test_input_validation(void)
{
    SpdTimingInputs timings;

    CHECK(
        spd_timing_inputs_check(NULL) == SPD_TIMING_ENCODE_INVALID_ARGUMENT); 

    timings = make_valid_timings();

    CHECK(
        spd_timing_inputs_check(&timings) == SPD_TIMING_ENCODE_OK);

    timings = make_valid_timings();
    timings.trfc1_min.known = false;

    CHECK(
        spd_timing_inputs_check(&timings) == SPD_TIMING_ENCODE_INCOMPLETE);

    timings = make_valid_timings();
    timings.taa_min.ps = UINT64_C(0);

    CHECK(
        spd_timing_inputs_check(&timings) == SPD_TIMING_ENCODE_INVALID_VALUE);

    timings.trfc1_min.known = false;

    CHECK(
        spd_timing_inputs_check(&timings) == SPD_TIMING_ENCODE_INVALID_VALUE);

    return 0;
}

static int test_timing_argument_validation(void)
{
    SpdTimingInputs timings;
    SpdImage image = SPD_IMAGE_INITIALIZER;

    timings = make_valid_timings();

    CHECK(
        spd_timing_encode(NULL, &image) == SPD_TIMING_ENCODE_INVALID_ARGUMENT);

    CHECK(
        spd_timing_encode(&timings, NULL) == SPD_TIMING_ENCODE_INVALID_ARGUMENT);

    CHECK(
        spd_timing_encode(&timings, &image) == SPD_TIMING_ENCODE_NOT_INITIALIZED); 

    return 0;
}

static int test_actual_timing_encoding(void)
{
    SpdTimingInputs timings;
    SpdImage image = SPD_IMAGE_INITIALIZER;

    timings = make_valid_timings();

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0, sizeof image.bytes);

    CHECK(
        spd_timing_encode(&timings, &image) == SPD_TIMING_ENCODE_OK);

    /* Timebase byte. */ 
    CHECK(image.bytes[17] == UINT8_C(0x00));

    /* tCKAVGmin:  625 / 125 = 5 = 0x05 */
    CHECK(image.bytes[18] == UINT8_C(0x05));
    CHECK(image.bytes[125] == UINT8_C(0x00));

    /* tAA: 13750 / 125 = 110 = 0x6E */
    CHECK(image.bytes[24] == UINT8_C(0x6E));
    CHECK(image.bytes[123] == UINT8_C(0x00));

    CHECK(image.bytes[25] == UINT8_C(0x6E));
    CHECK(image.bytes[122] == UINT8_C(0x00));

    CHECK(image.bytes[26] == UINT8_C(0x6E));
    CHECK(image.bytes[121] == UINT8_C(0x00));

    /* tRAS: 32000 / 125 = 256 = 0x100
     tRC: 45750 / 125 = 366 = 0x16E */

    CHECK(image.bytes[27] == UINT8_C(0x11));
    CHECK(image.bytes[28] == UINT8_C(0x00));
    CHECK(image.bytes[29] == UINT8_C(0x6E));
    CHECK(image.bytes[120] == UINT8_C(0x00));

    /* tRFC1: 350000 / 125 = 2800 = 0x0AF0 */
    CHECK(image.bytes[30] == UINT8_C(0xF0));
    CHECK(image.bytes[31] == UINT8_C(0x0A));

    /* tRFC2: 260000 / 125 = 2080 = 0x0820 */
    CHECK(image.bytes[32] == UINT8_C(0x20));
    CHECK(image.bytes[33] == UINT8_C(0x08));

    /* tRFC4: 160000 / 125 = 1280 = 0x0500 */
    CHECK(image.bytes[34] == UINT8_C(0x00));
    CHECK(image.bytes[35] == UINT8_C(0x05));

    /* tFAW: 30000 / 125 = 240 = 0x00F0 */ 
    CHECK((image.bytes[36] & UINT8_C(0x0F)) == UINT8_C(0x00));

    CHECK(image.bytes[37] == UINT8_C(0xF0));

    /* tRRD_S: 5000 / 125 = 40 = 0x28 */
    CHECK(image.bytes[38] == UINT8_C(0x28));
    CHECK(image.bytes[119] == UINT8_C(0x00));

    /* tRRD_L: 7500 / 125 = 60 = 0x3C */
    CHECK(image.bytes[39] == UINT8_C(0x3C));
    CHECK(image.bytes[118] == UINT8_C(0x00));

    /* tCCD_L: 6250 / 125 = 50 = 0x32 */
    CHECK(image.bytes[40] == UINT8_C(0x32));
    CHECK(image.bytes[117] == UINT8_C(0x00));

    return 0;
}

static int test_cas_encoding(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    CHECK(
        spd_cas_encode(NULL, &image) == SPD_TIMING_ENCODE_INVALID_ARGUMENT); 

    CHECK(
        spd_cas_encode("CL20 CL22", NULL) == SPD_TIMING_ENCODE_INVALID_ARGUMENT);

    CHECK(
        spd_cas_encode("CL20 CL22", &image) == SPD_TIMING_ENCODE_NOT_INITIALIZED);

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0, sizeof image.bytes);

    CHECK(
        spd_cas_encode("", &image) == SPD_TIMING_ENCODE_INCOMPLETE);

    CHECK(
        spd_cas_encode("   ", &image) == SPD_TIMING_ENCODE_INCOMPLETE);

    CHECK(
        spd_cas_encode("CL20 CL22", &image) ==  SPD_TIMING_ENCODE_OK);

    /* CL20 -> byte 21 bit 5 CL22 -> byte 21 bit 7 
      0x20 | 0x80 = 0xA0 */ 
    CHECK(image.bytes[20] == UINT8_C(0x00));
    CHECK(image.bytes[21] == UINT8_C(0xA0));
    CHECK(image.bytes[22] == UINT8_C(0x00));

    CHECK(
        (image.bytes[23] & UINT8_C(0x3F)) == UINT8_C(0x00));

    return 0;
}

static int test_invalid_cas(void)
{
    SpdImage image = SPD_IMAGE_INITIALIZER;

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    CHECK(
        spd_cas_encode("CL6", &image) == SPD_TIMING_ENCODE_INVALID_VALUE);

    CHECK(
        spd_cas_encode("CL37", &image) == SPD_TIMING_ENCODE_INVALID_VALUE);

    CHECK(
        spd_cas_encode("CLXX", &image) == SPD_TIMING_ENCODE_INVALID_VALUE);

    return 0;
}

static int test_output_preservation_on_failure(void)
{
    SpdTimingInputs timings;
    SpdImage image = SPD_IMAGE_INITIALIZER;
    uint8_t before[SPD_IMAGE_SIZE];

    timings = make_valid_timings();

    CHECK(
        spd_image_init(&image) == SPD_ENCODER_OK);

    memset(image.bytes, 0xA5, sizeof image.bytes);

    memcpy(before, image.bytes, sizeof before); 

    /* Force validation failure.*/ 
    timings.trfc1_min.known = false;

    CHECK(
        spd_timing_encode(&timings, &image) == SPD_TIMING_ENCODE_INCOMPLETE); 

    /* Failure must preserve original image. */
    CHECK(
        memcmp(before, image.bytes, sizeof before) == 0); 

    return 0;
}

static int test_status_strings(void)
{
    CHECK(
        strcmp(spd_timing_encode_status_string(SPD_TIMING_ENCODE_OK), "OK") == 0); 

    CHECK(
        strcmp(spd_timing_encode_status_string(
                SPD_TIMING_ENCODE_INVALID_ARGUMENT), "INVALID_ARGUMENT") == 0); 

    CHECK(
        strcmp(spd_timing_encode_status_string(
                SPD_TIMING_ENCODE_NOT_INITIALIZED), "NOT_INITIALIZED") == 0);

    CHECK(
        strcmp(
            spd_timing_encode_status_string(
                SPD_TIMING_ENCODE_INCOMPLETE), "INCOMPLETE") == 0); 

    CHECK(
        strcmp(
            spd_timing_encode_status_string(
                SPD_TIMING_ENCODE_INVALID_VALUE), "INVALID_VALUE") == 0); 

    CHECK(
        strcmp(
            spd_timing_encode_status_string(
                SPD_TIMING_ENCODE_MISSING_SPECIFICATION), 
            "MISSING_SPECIFICATION") == 0); 

    return 0;
}

int main(void)
{
    CHECK(test_input_validation() == 0);

    printf(
        "Timing input validation             : PASS\n");

    printf(
        "Unknown timing handling             : PASS\n");

    printf(
        "Known-zero timing rejection         : PASS\n");

    printf(
        "Invalid-value precedence            : PASS\n");

    CHECK(test_timing_argument_validation() == 0);

    printf(
        "Timing argument validation          : PASS\n");

    CHECK(test_actual_timing_encoding() == 0);

    printf(
        "MTB / FTB timing conversion         : PASS\n");

    printf(
        "tCKAVGmin encoding                  : PASS\n");

    printf(
        "tAA / tRCD / tRP encoding           : PASS\n");

    printf(
        "tRAS / tRC encoding                 : PASS\n");

    printf(
        "tRFC1 / tRFC2 / tRFC4 encoding      : PASS\n");

    printf(
        "tFAW encoding                       : PASS\n");

    printf(
        "tRRD_S / tRRD_L encoding            : PASS\n");

    printf(
        "tCCD_L encoding                     : PASS\n");

    printf(
        "Fine timing offset encoding         : PASS\n");

    CHECK(test_cas_encoding() == 0);

    printf(
        "CAS input validation                : PASS\n");

    printf(
        "CAS latency bitmap generation       : PASS\n");

    printf(
        "CL20 / CL22 expected bitmap         : 00 A0 00 00 PASS\n");

    CHECK(test_invalid_cas() == 0);

    printf(
        "Invalid CAS rejection               : PASS\n");

    CHECK(test_output_preservation_on_failure() == 0);

    printf(
        "Timing output preservation          : PASS\n");

    CHECK(test_status_strings() == 0);

    printf(
        "Status string validation            : PASS\n");

    printf("\n");

    printf(
        "Actual DDR4 timing byte encoding    : PASS\n");

    printf(
        "CAS latency bitmap encoding         : PASS\n");

    printf(
        "Implemented timing/CAS tests        : ALL PASS\n");

    return 0;
}         