/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "xf_autowhitebalance_config.h"

extern "C" {
void autowhitebalance_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                            ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                            float thresh,
                            int rows,
                            int cols) {
    // clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_inp1  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem3

#pragma HLS INTERFACE s_axilite port=thresh     bundle=control
#pragma HLS INTERFACE s_axilite port=rows     bundle=control
#pragma HLS INTERFACE s_axilite port=cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control
    // clang-format on

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> in_mat;
    // clang-format off
#pragma HLS stream variable=in_mat.data depth=2
    // clang-format on
    in_mat.rows = rows;
    in_mat.cols = cols;

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> in_mat1;
    // clang-format off
#pragma HLS stream variable=in_mat1.data depth=2
    // clang-format on
    in_mat1.rows = rows;
    in_mat1.cols = cols;

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> out_mat;
    // clang-format off
#pragma HLS stream variable=out_mat.data depth=2
    // clang-format on
    out_mat.rows = rows;
    out_mat.cols = cols;

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(img_inp, in_mat);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(img_inp1, in_mat1);

    xf::cv::balanceWhite<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_WB_TYPE>(in_mat, in_mat1, out_mat, thresh);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(out_mat, img_out);
}
}
