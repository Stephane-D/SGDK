package sgdk.aplib;

class Constant
{
    public static int[] prefixcode_literal = new int[] {0};
    public static int[] prefixcode_normal_match = new int[] {1, 0};
    public static int[] prefixcode_short_match = new int[] {1, 1, 0};
    public static int[] prefixcode_byte_match = new int[] {1, 1, 1};

    public static final int bitcost_prefixcode_literal = 1;
    public static final int bitcost_prefixcode_normalmatch = 2;
    public static final int bitcost_prefixcode_shortmatch = 3;
    public static final int bitcost_prefixcode_bytematch = 3;
    public static final int bitcost_offset_bytematch = 4;
    public static final int bitcost_offset_shortmatch = 8;
    public static final int bitcost_literal = 8;

    public static final int threshold_bytematch_offset = 16;
    public static final int threshold_shortmatch_offset = 128;
    public static final int threshold_length2match_offset = 1280;
    public static final int threshold_length3match_offset = 32000;
    public static final int threshold_offset = 65024;
    public static final int threshold_greedy_length = 65535; // 255; a workaround for a bug that kills compression on some
                                                      // highly compressible files
    public static final int threshold_gap_length = 24;

    public static final int repeat_offset_gap_bitcost_maxdeviation = 24;
    public static final int repeat_offset_edge_bitcost_maxdeviation = 18;
}
