#include <cstdio>
#include <cstddef>

extern "C"
{
#include <jpeglib.h>
}

#include "JpegCodec.h"

size_t
JPEGCodec::decode(const std::vector<uint8_t>& src, uint8_t* dst_data)
{
    auto jpeg_fail = [](j_common_ptr){};
    auto do_nothing = [](j_decompress_ptr){};

    jpeg_decompress_struct cinfo;
    jpeg_error_mgr error_mgr;
    error_mgr.error_exit = jpeg_fail;
    cinfo.err = jpeg_std_error(&error_mgr);
    jpeg_create_decompress(&cinfo);
    jpeg_source_mgr src_mgr;
    cinfo.src = &src_mgr;

    // Prepare for suspending reader
    src_mgr.init_source = do_nothing;
    src_mgr.resync_to_restart = jpeg_resync_to_restart;
    src_mgr.term_source = do_nothing;
    src_mgr.next_input_byte = src.data();
    src_mgr.bytes_in_buffer = src.size();

    jpeg_read_header(&cinfo, TRUE);
    jpeg_calc_output_dimensions(&cinfo);
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;

    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width * 4, 1);

    for(; height--; dst_data += (width * 3))
    {
      jpeg_read_scanlines(&cinfo, buffer, 1);
      memcpy(dst_data, buffer[0], width * 3);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return height * width * 3;
}
