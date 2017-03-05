#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "gsth264parser.h"
#include "nalutils.h"

static const gchar* 
get_nal_type_desc(GstH264NalUnitType type)
{
    const gchar *retval = NULL;
    switch(type) {
        case GST_H264_NAL_UNKNOWN:
            retval = "GST_H264_NAL_UNKNOWN";
            break;
        case GST_H264_NAL_SLICE:
            retval = "GST_H264_NAL_SLICE";
            break;
        case GST_H264_NAL_SLICE_DPA:
            retval = "GST_H264_NAL_SLICE_DPA";
            break;
        case GST_H264_NAL_SLICE_DPB:
            retval = "GST_H264_NAL_SLICE_DPB";
            break;
        case GST_H264_NAL_SLICE_DPC:
            retval = "GST_H264_NAL_SLICE_DPC";
            break;
        case GST_H264_NAL_SLICE_IDR:
            retval = "GST_H264_NAL_SLICE_IDR";
            break;
        case GST_H264_NAL_SEI:
            retval = "GST_H264_NAL_SEI";
            break;
        case GST_H264_NAL_SPS:
            retval = "GST_H264_NAL_SPS";
            break;
        case GST_H264_NAL_PPS:
            retval = "GST_H264_NAL_PPS";
            break;
        case GST_H264_NAL_AU_DELIMITER:
            retval = "GST_H264_NAL_AU_DELIMITER";
            break;
        case GST_H264_NAL_SEQ_END:
            retval = "GST_H264_NAL_SEQ_END";
            break;
        case GST_H264_NAL_STREAM_END:
            retval = "GST_H264_NAL_STREAM_END";
            break;
        case GST_H264_NAL_FILLER_DATA:
            retval = "GST_H264_NAL_FILLER_DATA";
            break;
        case GST_H264_NAL_SPS_EXT:
            retval = "GST_H264_NAL_SPS_EXT";
            break;
        case GST_H264_NAL_PREFIX_UNIT:
            retval = "GST_H264_NAL_PREFIX_UNIT";
            break;
        case GST_H264_NAL_SUBSET_SPS:
            retval = "GST_H264_NAL_SUBSET_SPS";
            break;
        case GST_H264_NAL_DEPTH_SPS:
            retval = "GST_H264_NAL_DEPTH_SPS";
            break;
        case GST_H264_NAL_SLICE_AUX:
            retval = "GST_H264_NAL_SLICE_AUX";
            break;
        case GST_H264_NAL_SLICE_EXT:
            retval = "GST_H264_NAL_SLICE_EXT";
            break;
        case GST_H264_NAL_SLICE_DEPTH:
            retval = "GST_H264_NAL_SLICE_DEPTH";
            break;
        default:
            retval = "<<unknown>>";
            break;
    }
    return retval;
}

static GstH264NalUnit nalu;

//static int
//test0(int argc, char** argv)
//{
//    if(argc<2) {
//        return EXIT_FAILURE;
//    }
//    
//    GError *error = NULL;
//    gint exit_code = EXIT_SUCCESS;
//    guint offset = 0;
//    
//    GstH264NalParser* parser = gst_h264_nal_parser_new();
//    g_print("sample file: [%s]\n", argv[1]);
//    GMappedFile *mf = g_mapped_file_new(argv[1], FALSE, &error);
//    if(!mf) {
//        exit_code = EXIT_FAILURE;
//        goto clean_exit;
//    }
//    GBytes *body = g_mapped_file_get_bytes(mf);
//    gsize sz = -1;
//    gconstpointer body_data = g_bytes_get_data(body, &sz);
//    g_print("size: %d\n", sz);
//    
//    GstH264ParserResult result;
//    
//    while(offset<sz) {
//        result = gst_h264_parser_identify_nalu(parser, body_data, offset, sz, &nalu);
//        g_print("result: %d ", result);
//        offset = nalu.offset + nalu.size;
//        if(result==GST_H264_PARSER_OK) {
//            g_print("offset=%-10d, size=%-10d, ref_idc=%-2d, type=%-25s(%-2d)"
//                  ", idr_pic_flag=%d, valid=%d, header_bytes=%-2d"
//                  ", extension_type=%-2d"
//              , nalu.offset
//              , nalu.size
//              , nalu.ref_idc
//              , get_nal_type_desc(nalu.type) // GstH264NalUnitType
//              , nalu.type
//              , nalu.idr_pic_flag
//              , nalu.valid
//              , nalu.header_bytes
//              , nalu.extension_type
//            );
//        }
//        else {
//            g_print("offset=%-10d, size=%-10d", nalu.offset, nalu.size);
//        }
//        g_print("\n");
//    }
//    g_print("loop end: %d < %d\n", offset, sz);
//    
////    result = gst_h264_parser_identify_nalu(parser, body_data, offset, sz, &nalu);
////    g_print("result: %d\n", result);
////    g_print("nalu offset: %d\n", nalu.offset);
////    g_print("nalu size: %d\n", nalu.size);
//    
//clean_exit:
//    gst_h264_nal_parser_free(parser);
//    g_mapped_file_unref(mf);
//    g_bytes_unref(body);
//    return exit_code;
//}

static int
test1(int argc, char** argv)
{
    if(argc<2) {
        return EXIT_FAILURE;
    }
    
    GError *error = NULL;
    gint exit_code = EXIT_SUCCESS;
    guint offset = 0;
    
    GstH264NalParser* parser = gst_h264_nal_parser_new();
    g_print("sample file: [%s]\n", argv[1]);
    GMappedFile *mf = g_mapped_file_new(argv[1], FALSE, &error);
    if(!mf) {
        exit_code = EXIT_FAILURE;
        goto clean_exit;
    }
    GBytes *body = g_mapped_file_get_bytes(mf);
    gsize sz = -1;
    gconstpointer body_data = g_bytes_get_data(body, &sz);
    g_print("size: %zu\n", sz);
    
    GByteArray *ba = g_byte_array_new();
    GRand *rand = g_rand_new();
    gint data_len = -1;
    const guint8 *data = NULL;
    gint scan_result1 = -1, scan_result2 = -1;
    GstH264ParserResult result;
    while(offset<sz)
    {
        data = body_data+offset;
        data_len = MIN(g_rand_int_range(rand, 1, 3000), sz-offset);
        // this will copy data memory to internal store
        g_byte_array_append(ba, data, data_len);
        
//        g_print("ba len = %d\n", ba->len);
        
        scan_result1 = scan_for_start_codes(ba->data, ba->len);
//        g_print("scan_result1: (%d) %d, %d\n", offset, data_len, scan_result1);
        
        if(scan_result1>=0) {
            scan_result2 = scan_for_start_codes(ba->data+scan_result1+3, ba->len-scan_result1);
            if(scan_result2>=0) {
                scan_result2 += 3;
            }
//            g_print("scan_result2: (%d) %d, %d\n", offset, data_len, scan_result2);
        }
        
        if(scan_result1>=0)
        {
            if(scan_result2>0) {
                g_assert_true(scan_result2>scan_result1);
            }
            if(scan_result2>0 || offset+data_len >= sz) {
                result = gst_h264_parser_identify_nalu(parser, ba->data, 0, ba->len, &nalu);
                if(result==GST_H264_PARSER_OK)
                {
                    g_print("offset=%-10d, size=%-10d, ref_idc=%-2d, type=%-25s(%-2d)"
                          ", idr_pic_flag=%d, valid=%d, header_bytes=%-2d"
                          ", extension_type=%-2d\n"
                      , nalu.offset
                      , nalu.size
                      , nalu.ref_idc
                      , get_nal_type_desc(nalu.type) // GstH264NalUnitType
                      , nalu.type
                      , nalu.idr_pic_flag
                      , nalu.valid
                      , nalu.header_bytes
                      , nalu.extension_type
                    );
                    // остаток комируем в ba, а старое уничтожаем
                    g_byte_array_remove_range(ba, 0, scan_result2);
                    scan_result1 = -1;
                    scan_result2 = -1;
                }
                else {
                    // gst_h264_parser_identify_nalu does not know how to handle end of stream
                    if(offset+data_len >= sz) {
                        g_print("offset=%-10d, size=%-10d, ref_idc=%-2d, type=%-25s(%-2d)"
                              ", idr_pic_flag=%d, valid=%d, header_bytes=%-2d"
                              ", extension_type=%-2d\n"
                          , nalu.offset
                          , nalu.size
                          , nalu.ref_idc
                          , get_nal_type_desc(nalu.type) // GstH264NalUnitType
                          , nalu.type
                          , nalu.idr_pic_flag
                          , nalu.valid
                          , nalu.header_bytes
                          , nalu.extension_type
                        );
                    }
                    else {
                        g_print("parser failed!\n");
                    }
                }
                memset(&nalu, 0, sizeof(GstH264NalUnit));
            }
        }
        
        offset += data_len;
    }
    
clean_exit:
    gst_h264_nal_parser_free(parser);
    g_mapped_file_unref(mf);
    g_bytes_unref(body);
    g_rand_free(rand);
    return exit_code;
}

int
gvb_h264_parser_test(int argc, char** argv)
{
    return test1(argc, argv);
}
