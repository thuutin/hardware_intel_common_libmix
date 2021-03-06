#include <vbp_common.h>
#include <vbp_trace.h>
#include "viddec_mp4_visualobject.h"

static inline uint8_t mp4_pvt_isValid_verID(uint8_t id)
{
    uint8_t ret=true;
    switch (id)
    {
    case 1:
    case 2:
    case 4:
    case 5:
    {
        break;
    }
    default:
    {
        ret = false;
        break;
    }
    }
    return ret;
} // mp4_pvt_isValid_verID

static mp4_Status_t mp4_Parse_video_signal_type(void *parent, mp4_VideoSignalType_t *vidSignal)
{
    uint32_t data=0;
    int32_t getbits=0;
    mp4_Status_t ret = MP4_STATUS_PARSE_ERROR;

    /* Set default values defined in spec first */
    vidSignal->video_format = 5;
    vidSignal->video_range = 0;
    vidSignal->colour_primaries = 1;
    vidSignal->transfer_characteristics = 1;
    vidSignal->matrix_coefficients = 1;
    do
    {
        getbits = viddec_pm_get_bits(parent, &data, 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        vidSignal->is_video_signal_type = (data > 0);
        if (vidSignal->is_video_signal_type)
        {
            getbits = viddec_pm_get_bits(parent, &data, 5);
            BREAK_GETBITS_FAIL(getbits, ret);
            vidSignal->is_colour_description = data & 0x1;
            vidSignal->video_range = ((data & 0x2) > 0);
            data =  data >> 2;
            vidSignal->video_format = data & 0x7;
            if (vidSignal->is_colour_description)
            {
                getbits = viddec_pm_get_bits(parent, &data, 24);
                BREAK_GETBITS_FAIL(getbits, ret);
                vidSignal->colour_primaries = (data >> 16) & 0xFF;
                vidSignal->transfer_characteristics = (data >> 8) & 0xFF;
                vidSignal->matrix_coefficients = data & 0xFF;
            }
        }
        ret = MP4_STATUS_OK;
    } while (0);

    return ret;
} // mp4_Parse_video_signal_type

void mp4_set_hdr_bitstream_error(viddec_mp4_parser_t *parser, uint8_t hdr_flag, mp4_Status_t parse_status)
{
    //VTRACE("Entering mp4_set_hdr_bitstream_error: bs_err: 0x%x, hdr: %d, parse_status: %d\n",
    //  parser->bitstream_error, hdr_flag, parse_status);

    if (hdr_flag)
    {
        if (parse_status & MP4_STATUS_NOTSUPPORT)
            parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
        if (parse_status & MP4_STATUS_PARSE_ERROR)
            parser->bitstream_error |= MP4_BS_ERROR_HDR_PARSE;
        if (parse_status & MP4_STATUS_REQD_DATA_ERROR)
            parser->bitstream_error |= MP4_BS_ERROR_HDR_NONDEC;
        parser->bitstream_error &= MP4_HDR_ERROR_MASK;
    }
    else
    {
        if (parse_status & MP4_STATUS_NOTSUPPORT)
            parser->bitstream_error |= MP4_BS_ERROR_FRM_UNSUP;
        if (parse_status & MP4_STATUS_PARSE_ERROR)
            parser->bitstream_error |= MP4_BS_ERROR_FRM_PARSE;
        if (parse_status & MP4_STATUS_REQD_DATA_ERROR)
            parser->bitstream_error |= MP4_BS_ERROR_FRM_NONDEC;
    }

    //VTRACE("Exiting mp4_set_hdr_bitstream_error: bs_err: 0x%x\n", parser->bitstream_error);

    return;
} // mp4_set_hdr_bitstream_error

mp4_Status_t mp4_Parse_VisualSequence(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t data=0;
    int32_t getbits=0;
    mp4_Status_t ret = MP4_STATUS_PARSE_ERROR;

    getbits = viddec_pm_get_bits(parent, &data, 8);
    if (getbits != -1)
    {
        parser->info.profile_and_level_indication = data & 0xFF;
        // If present, check for validity
        switch (parser->info.profile_and_level_indication)
        {
        case MP4_SIMPLE_PROFILE_LEVEL_0:
        case MP4_SIMPLE_PROFILE_LEVEL_1:
        case MP4_SIMPLE_PROFILE_LEVEL_2:
        case MP4_SIMPLE_PROFILE_LEVEL_3:
        case MP4_SIMPLE_PROFILE_LEVEL_4a:
        case MP4_SIMPLE_PROFILE_LEVEL_5:
        case MP4_SIMPLE_PROFILE_LEVEL_6:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_0:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_1:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_2:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_4:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_5:
        case MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3B:
            parser->bitstream_error = MP4_BS_ERROR_NONE;
            ret = MP4_STATUS_OK;
            break;
        default:
            parser->bitstream_error = MP4_BS_ERROR_HDR_UNSUP | MP4_BS_ERROR_HDR_NONDEC;
            break;
        }
    }
    else
    {
        parser->bitstream_error = MP4_BS_ERROR_HDR_PARSE | MP4_BS_ERROR_HDR_NONDEC;
    }

    return ret;
} // mp4_Parse_VisualSequence

mp4_Status_t mp4_Parse_VisualObject(void *parent, viddec_mp4_parser_t *parser)
{
    mp4_Info_t *pInfo = &(parser->info);
    mp4_VisualObject_t *visObj = &(pInfo->VisualObject);
    uint32_t data=0;
    int32_t getbits=0;
    mp4_Status_t ret = MP4_STATUS_PARSE_ERROR;

    do
    {
        getbits = viddec_pm_get_bits(parent, &data, 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        visObj->is_visual_object_identifier = (data > 0);

        visObj->visual_object_verid = 1; /* Default value as per spec */
        if (visObj->is_visual_object_identifier)
        {
            viddec_pm_get_bits(parent, &data, 7);
            visObj->visual_object_priority = data & 0x7;
            data = data >> 3;
            if (mp4_pvt_isValid_verID(data & 0xF))
            {
                visObj->visual_object_verid = data & 0xF;
            }
            else
            {
                ETRACE("Warning: Unsupported visual_object_verid\n");
                parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
                // Continue parsing as it is not a required field for decoder
            }
        }

        getbits = viddec_pm_get_bits(parent, &data, 4);
        BREAK_GETBITS_FAIL(getbits, ret);
        visObj->visual_object_type = data;
        if (visObj->visual_object_type != MP4_VISUAL_OBJECT_TYPE_VIDEO)
        {
            /* VIDEO is the only supported type */
            ETRACE("Error: Unsupported object: visual_object_type != video ID\n");
            parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
            break;
        }

        /* Not required to check for visual_object_type as we already handle it above */
        ret = mp4_Parse_video_signal_type(parent, &(visObj->VideoSignalType));

        // No need to check for user data or visual object layer because they have a different start code
        // and will not be part of this header

    } while (0);

    mp4_set_hdr_bitstream_error(parser, true, ret);

    return ret;
} // mp4_Parse_VisualObject


