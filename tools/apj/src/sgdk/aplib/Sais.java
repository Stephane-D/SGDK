package sgdk.aplib;

class SuffixArray
{
    interface BaseArray
    {
        public int get(int off);

        public void set(int off, int value);
    }

    static class ByteArray implements BaseArray
    {
        private byte[] m_array;
        private int m_pos;

        public ByteArray(byte[] array, int pos)
        {
            m_pos = pos;
            m_array = array;
        }

        ByteArray()
        {
            m_array = null;
        }

        @Override
        public int get(int off)
        {
            return m_array[off + m_pos] & 0xFF;
        }

        @Override
        public void set(int off, int value)
        {
            m_array[off + m_pos] = (byte) value;
        }
    }

    static class CharArray implements BaseArray
    {
        private char[] m_array;
        private int m_pos;

        public CharArray(char[] array, int pos)
        {
            m_pos = pos;
            m_array = array;
        }

        CharArray()
        {
            m_array = null;
        }

        @Override
        public int get(int off)
        {
            return m_array[off + m_pos] & 0xFFFF;
        }

        @Override
        public void set(int off, int value)
        {
            m_array[off + m_pos] = (char) value;
        }
    }

    static class IntArray implements BaseArray
    {
        private int[] m_array;
        private int m_pos;

        public IntArray(int[] array, int pos)
        {
            m_pos = pos;
            m_array = array;
        }

        IntArray()
        {
            m_array = null;
        }

        @Override
        public int get(int off)
        {
            return m_array[off + m_pos];
        }

        @Override
        public void set(int off, int value)
        {
            m_array[off + m_pos] = value;
        }
    }

    static class StringArray implements BaseArray
    {
        private String m_array;
        private int m_pos;

        public StringArray(String array, int pos)
        {
            m_pos = pos;
            m_array = array;
        }

        StringArray()
        {
            m_array = null;
        }

        @Override
        public int get(int off)
        {
            return (int) m_array.charAt(off + m_pos);
        }

        @Override
        public void set(int off, int value)
        {
        }
    }

    /// <summary>
    /// An implementation of the induced sorting based suffix array construction algorithm.
    /// </summary>
    public static class SAIS
    {
        private final static int MINBUCKETSIZE = 256;

        private static void getCounts(BaseArray T, BaseArray C, int n, int k)
        {
            int i;

            for (i = 0; i < k; ++i)
                C.set(i, 0);

            for (i = 0; i < n; ++i)
                C.set(T.get(i), C.get(T.get(i)) + 1);
        }

        private static void getBuckets(BaseArray C, BaseArray B, int k, boolean end)
        {
            int i, sum = 0;
            if (end != false)
            {
                for (i = 0; i < k; ++i)
                {
                    sum += C.get(i);
                    B.set(i, sum);
                }
            }
            else
            {
                for (i = 0; i < k; ++i)
                {
                    sum += C.get(i);
                    B.set(i, sum - C.get(i));
                }
            }
        }

        /* sort all type LMS suffixes */
        private static void LMSsort(BaseArray T, int[] SA, BaseArray C, BaseArray B, int n, int k)
        {
            int b, i, j;
            int c0, c1;
            /* compute SAl */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, false); /* find starts of buckets */
            j = n - 1;
            b = B.get(c1 = T.get(j));
            --j;
            SA[b++] = (T.get(j) < c1) ? ~j : j;
            for (i = 0; i < n; ++i)
            {
                if (0 < (j = SA[i]))
                {
                    if ((c0 = T.get(j)) != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    --j;
                    SA[b++] = (T.get(j) < c1) ? ~j : j;
                    SA[i] = 0;
                }
                else if (j < 0)
                {
                    SA[i] = ~j;
                }
            }
            /* compute SAs */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, true); /* find ends of buckets */
            for (i = n - 1, b = B.get(c1 = 0); 0 <= i; --i)
            {
                if (0 < (j = SA[i]))
                {
                    if ((c0 = T.get(j)) != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    --j;
                    SA[--b] = (T.get(j) > c1) ? ~(j + 1) : j;
                    SA[i] = 0;
                }
            }
        }

        private static int LMSpostproc(BaseArray T, int[] SA, int n, int m)
        {
            int i, j, p, q, plen, qlen, name;
            int c0, c1;
            boolean diff;

            /*
             * compact all the sorted subStrings into the first m items of SA
             * 2*m must be not larger than n (proveable)
             */
            for (i = 0; (p = SA[i]) < 0; ++i)
            {
                SA[i] = ~p;
            }
            if (i < m)
            {
                for (j = i, ++i;; ++i)
                {
                    if ((p = SA[i]) < 0)
                    {
                        SA[j++] = ~p;
                        SA[i] = 0;
                        if (j == m)
                        {
                            break;
                        }
                    }
                }
            }

            /* store the length of all subStrings */
            i = n - 1;
            j = n - 1;
            c0 = T.get(n - 1);
            do
            {
                c1 = c0;
            }
            while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
            for (; 0 <= i;)
            {
                do
                {
                    c1 = c0;
                }
                while ((0 <= --i) && ((c0 = T.get(i)) <= c1));
                if (0 <= i)
                {
                    SA[m + ((i + 1) >> 1)] = j - i;
                    j = i + 1;
                    do
                    {
                        c1 = c0;
                    }
                    while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
                }
            }

            /* find the lexicographic names of all subStrings */
            for (i = 0, name = 0, q = n, qlen = 0; i < m; ++i)
            {
                p = SA[i];
                plen = SA[m + (p >> 1)];
                diff = true;
                if ((plen == qlen) && ((q + plen) < n))
                {
                    for (j = 0; (j < plen) && (T.get(p + j) == T.get(q + j)); ++j)
                    {
                    }
                    if (j == plen)
                    {
                        diff = false;
                    }
                }
                if (diff != false)
                {
                    ++name;
                    q = p;
                    qlen = plen;
                }
                SA[m + (p >> 1)] = name;
            }

            return name;
        }

        /* compute SA and BWT */
        private static void induceSA(BaseArray T, int[] SA, BaseArray C, BaseArray B, int n, int k)
        {
            int b, i, j;
            int c0, c1;
            /* compute SAl */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, false); /* find starts of buckets */
            j = n - 1;
            b = B.get(c1 = T.get(j));
            SA[b++] = ((0 < j) && (T.get(j - 1) < c1)) ? ~j : j;
            for (i = 0; i < n; ++i)
            {
                j = SA[i];
                SA[i] = ~j;
                if (0 < j)
                {
                    if ((c0 = T.get(--j)) != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    SA[b++] = ((0 < j) && (T.get(j - 1) < c1)) ? ~j : j;
                }
            }
            /* compute SAs */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, true); /* find ends of buckets */
            for (i = n - 1, b = B.get(c1 = 0); 0 <= i; --i)
            {
                if (0 < (j = SA[i]))
                {
                    if ((c0 = T.get(--j)) != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    SA[--b] = ((j == 0) || (T.get(j - 1) > c1)) ? ~j : j;
                }
                else
                {
                    SA[i] = ~j;
                }
            }
        }

        private static int computeBWT(BaseArray T, int[] SA, BaseArray C, BaseArray B, int n, int k)
        {
            int b, i, j, pidx = -1;
            int c0, c1;
            /* compute SAl */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, false); /* find starts of buckets */
            j = n - 1;
            b = B.get(c1 = T.get(j));
            SA[b++] = ((0 < j) && (T.get(j - 1) < c1)) ? ~j : j;
            for (i = 0; i < n; ++i)
            {
                if (0 < (j = SA[i]))
                {
                    SA[i] = ~(c0 = T.get(--j));
                    if (c0 != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    SA[b++] = ((0 < j) && (T.get(j - 1) < c1)) ? ~j : j;
                }
                else if (j != 0)
                {
                    SA[i] = ~j;
                }
            }
            /* compute SAs */
            if (C == B)
            {
                getCounts(T, C, n, k);
            }
            getBuckets(C, B, k, true); /* find ends of buckets */
            for (i = n - 1, b = B.get(c1 = 0); 0 <= i; --i)
            {
                if (0 < (j = SA[i]))
                {
                    SA[i] = (c0 = T.get(--j));
                    if (c0 != c1)
                    {
                        B.set(c1, b);
                        b = B.get(c1 = c0);
                    }
                    SA[--b] = ((0 < j) && (T.get(j - 1) > c1)) ? ~((int) T.get(j - 1)) : j;
                }
                else if (j != 0)
                {
                    SA[i] = ~j;
                }
                else
                {
                    pidx = i;
                }
            }
            return pidx;
        }

        /*
         * find the suffix array SA of T[0..n-1] in {0..k-1}^n
         * use a working space (excluding T and SA) of at most 2n+O(1) for a constant alphabet
         */
        private static int sais_main(BaseArray T, int[] SA, int fs, int n, int k, boolean isbwt)
        {
            BaseArray C, B, RA;
            int i, j, b, m, p, q, name, pidx = 0, newfs;
            int c0, c1;
            int flags = 0;

            if (k <= MINBUCKETSIZE)
            {
                C = new IntArray(new int[k], 0);
                if (k <= fs)
                {
                    B = new IntArray(SA, n + fs - k);
                    flags = 1;
                }
                else
                {
                    B = new IntArray(new int[k], 0);
                    flags = 3;
                }
            }
            else if (k <= fs)
            {
                C = new IntArray(SA, n + fs - k);
                if (k <= (fs - k))
                {
                    B = new IntArray(SA, n + fs - k * 2);
                    flags = 0;
                }
                else if (k <= (MINBUCKETSIZE * 4))
                {
                    B = new IntArray(new int[k], 0);
                    flags = 2;
                }
                else
                {
                    B = C;
                    flags = 8;
                }
            }
            else
            {
                C = B = new IntArray(new int[k], 0);
                flags = 4 | 8;
            }

            /*
             * stage 1: reduce the problem by at least 1/2
             * sort all the LMS-subStrings
             */
            getCounts(T, C, n, k);
            getBuckets(C, B, k, true); /* find ends of buckets */
            for (i = 0; i < n; ++i)
            {
                SA[i] = 0;
            }
            b = -1;
            i = n - 1;
            j = n;
            m = 0;
            c0 = T.get(n - 1);
            do
            {
                c1 = c0;
            }
            while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
            for (; 0 <= i;)
            {
                do
                {
                    c1 = c0;
                }
                while ((0 <= --i) && ((c0 = T.get(i)) <= c1));
                if (0 <= i)
                {
                    if (0 <= b)
                    {
                        SA[b] = j;
                    }
                    B.set(c1, B.get(c1) - 1);
                    b = B.get(c1);
                    j = i;
                    ++m;
                    do
                    {
                        c1 = c0;
                    }
                    while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
                }
            }
            if (1 < m)
            {
                LMSsort(T, SA, C, B, n, k);
                name = LMSpostproc(T, SA, n, m);
            }
            else if (m == 1)
            {
                SA[b] = j + 1;
                name = 1;
            }
            else
            {
                name = 0;
            }

            /*
             * stage 2: solve the reduced problem
             * recurse if names are not yet unique
             */
            if (name < m)
            {
                if ((flags & 4) != 0)
                {
                    C = null;
                    B = null;
                }
                if ((flags & 2) != 0)
                {
                    B = null;
                }
                newfs = (n + fs) - (m * 2);
                if ((flags & (1 | 4 | 8)) == 0)
                {
                    if ((k + name) <= newfs)
                    {
                        newfs -= k;
                    }
                    else
                    {
                        flags |= 8;
                    }
                }
                for (i = m + (n >> 1) - 1, j = m * 2 + newfs - 1; m <= i; --i)
                {
                    if (SA[i] != 0)
                    {
                        SA[j--] = SA[i] - 1;
                    }
                }
                RA = new IntArray(SA, m + newfs);
                sais_main(RA, SA, newfs, m, name, false);
                RA = null;

                i = n - 1;
                j = m * 2 - 1;
                c0 = T.get(n - 1);
                do
                {
                    c1 = c0;
                }
                while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
                for (; 0 <= i;)
                {
                    do
                    {
                        c1 = c0;
                    }
                    while ((0 <= --i) && ((c0 = T.get(i)) <= c1));
                    if (0 <= i)
                    {
                        SA[j--] = i + 1;
                        do
                        {
                            c1 = c0;
                        }
                        while ((0 <= --i) && ((c0 = T.get(i)) >= c1));
                    }
                }

                for (i = 0; i < m; ++i)
                {
                    SA[i] = SA[m + SA[i]];
                }
                if ((flags & 4) != 0)
                {
                    C = B = new IntArray(new int[k], 0);
                }
                if ((flags & 2) != 0)
                {
                    B = new IntArray(new int[k], 0);
                }
            }

            /* stage 3: induce the result for the original problem */
            if ((flags & 8) != 0)
            {
                getCounts(T, C, n, k);
            }
            /* put all left-most S characters into their buckets */
            if (1 < m)
            {
                getBuckets(C, B, k, true); /* find ends of buckets */
                i = m - 1;
                j = n;
                p = SA[m - 1];
                c1 = T.get(p);
                do
                {
                    q = B.get(c0 = c1);
                    while (q < j)
                    {
                        SA[--j] = 0;
                    }
                    do
                    {
                        SA[--j] = p;
                        if (--i < 0)
                        {
                            break;
                        }
                        p = SA[i];
                    }
                    while ((c1 = T.get(p)) == c0);
                }
                while (0 <= i);
                while (0 < j)
                {
                    SA[--j] = 0;
                }
            }
            if (isbwt == false)
            {
                induceSA(T, SA, C, B, n, k);
            }
            else
            {
                pidx = computeBWT(T, SA, C, B, n, k);
            }
            C = null;
            B = null;
            return pidx;
        }

        /*- Suffixsorting -*/
        /* byte */
        /// <summary>
        /// Constructs the suffix array of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="SA">output suffix array</param>
        /// <param name="n">length of the given String</param>
        /// <returns>0 if no error occurred, -1 or -2 otherwise</returns>
        public static int sufsort(byte[] T, int[] SA, int n)
        {
            if ((T == null) || (SA == null) || (T.length < n) || (SA.length < n))
                return -1;

            if (n <= 1)
            {
                if (n == 1)
                    SA[0] = 0;

                return 0;
            }

            return sais_main(new ByteArray(T, 0), SA, 0, n, 256, false);
        }

        public static int[] sa(byte[] input) throws Exception
        {
            if (input == null || input.length < 1)
                throw new Exception("SAIS: input too small");

            int[] output = new int[input.length];
            sais_main(new ByteArray(input, 0), output, 0, input.length, 256, false);
            
            return output;
        }

        /* char */
        /// <summary>
        /// Constructs the suffix array of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="SA">output suffix array</param>
        /// <param name="n">length of the given String</param>
        /// <returns>0 if no error occurred, -1 or -2 otherwise</returns>
        public static int sufsort(char[] T, int[] SA, int n)
        {
            if ((T == null) || (SA == null) || (T.length < n) || (SA.length < n))
            {
                return -1;
            }
            if (n <= 1)
            {
                if (n == 1)
                {
                    SA[0] = 0;
                }
                return 0;
            }
            return sais_main(new CharArray(T, 0), SA, 0, n, 65536, false);
        }

        /* int */
        /// <summary>
        /// Constructs the suffix array of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="SA">output suffix array</param>
        /// <param name="n">length of the given String</param>
        /// <param name="k">alphabet size</param>
        /// <returns>0 if no error occurred, -1 or -2 otherwise</returns>
        public static int sufsort(int[] T, int[] SA, int n, int k)
        {
            if ((T == null) || (SA == null) || (T.length < n) || (SA.length < n) || (k <= 0))
            {
                return -1;
            }
            if (n <= 1)
            {
                if (n == 1)
                {
                    SA[0] = 0;
                }
                return 0;
            }
            return sais_main(new IntArray(T, 0), SA, 0, n, k, false);
        }

        /* String */
        /// <summary>
        /// Constructs the suffix array of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="SA">output suffix array</param>
        /// <param name="n">length of the given String</param>
        /// <returns>0 if no error occurred, -1 or -2 otherwise</returns>
        public static int sufsort(String T, int[] SA, int n)
        {
            if ((T == null) || (SA == null) || (T.length() < n) || (SA.length < n))
            {
                return -1;
            }
            if (n <= 1)
            {
                if (n == 1)
                {
                    SA[0] = 0;
                }
                return 0;
            }
            return sais_main(new StringArray(T, 0), SA, 0, n, 65536, false);
        }

        /*- Burrows-Wheeler Transform -*/
        /* byte */
        /// <summary>
        /// Constructs the burrows-wheeler transformed String of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="U">output String</param>
        /// <param name="A">temporary array</param>
        /// <param name="n">length of the given String</param>
        /// <returns>primary index if no error occurred, -1 or -2 otherwise</returns>
        public static int bwt(byte[] T, byte[] U, int[] A, int n)
        {
            int i, pidx;

            if ((T == null) || (U == null) || (A == null) || (T.length < n) || (U.length < n) || (A.length < n))
                return -1;

            if (n <= 1)
            {
                if (n == 1)
                    U[0] = T[0];

                return n;
            }

            pidx = sais_main(new ByteArray(T, 0), A, 0, n, 256, true);

            U[0] = T[n - 1];
            for (i = 0; i < pidx; ++i)
                U[i + 1] = (byte) A[i];
            for (i += 1; i < n; ++i)
                U[i] = (byte) A[i];

            return pidx + 1;
        }

        /* char */
        /// <summary>
        /// Constructs the burrows-wheeler transformed String of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="U">output String</param>
        /// <param name="A">temporary array</param>
        /// <param name="n">length of the given String</param>
        /// <returns>primary index if no error occurred, -1 or -2 otherwise</returns>
        public static int bwt(char[] T, char[] U, int[] A, int n)
        {
            int i, pidx;

            if ((T == null) || (U == null) || (A == null) || (T.length < n) || (U.length < n) || (A.length < n))
                return -1;

            if (n <= 1)
            {
                if (n == 1)
                    U[0] = T[0];

                return n;
            }

            pidx = sais_main(new CharArray(T, 0), A, 0, n, 65536, true);

            U[0] = T[n - 1];
            for (i = 0; i < pidx; ++i)
                U[i + 1] = (char) A[i];
            for (i += 1; i < n; ++i)
                U[i] = (char) A[i];

            return pidx + 1;
        }

        /* int */
        /// <summary>
        /// Constructs the burrows-wheeler transformed String of a given String in linear time.
        /// </summary>
        /// <param name="T">input String</param>
        /// <param name="U">output String</param>
        /// <param name="A">temporary array</param>
        /// <param name="n">length of the given String</param>
        /// <param name="k">alphabet size</param>
        /// <returns>primary index if no error occurred, -1 or -2 otherwise</returns>
        public static int bwt(int[] T, int[] U, int[] A, int n, int k)
        {
            int i, pidx;

            if ((T == null) || (U == null) || (A == null) || (T.length < n) || (U.length < n) || (A.length < n)
                    || (k <= 0))
                return -1;

            if (n <= 1)
            {
                if (n == 1)
                    U[0] = T[0];

                return n;
            }

            pidx = sais_main(new IntArray(T, 0), A, 0, n, k, true);

            U[0] = T[n - 1];
            for (i = 0; i < pidx; ++i)
                U[i + 1] = A[i];
            for (i += 1; i < n; ++i)
                U[i] = A[i];

            return pidx + 1;
        }

        public static int check(byte[] T, int[] SA, int n, boolean verbose)
        {
            int[] C = new int[256];
            int i, p, q, t;
            int c;

            if (verbose)
                System.out.print("sufcheck: ");

            if (n == 0)
            {
                if (verbose)
                    System.out.println("Done.");

                return 0;
            }

            /* Check arguments. */
            if ((T == null) || (SA == null) || (n < 0))
            {
                if (verbose)
                    System.out.println("Invalid arguments.");

                return -1;
            }

            /* check range: [0..n-1] */
            for (i = 0; i < n; ++i)
            {
                if ((SA[i] < 0) || (n <= SA[i]))
                {
                    if (verbose)
                    {
                        System.out.println("Out of the range [0," + (n - 1) + "].");
                        System.out.println("  SA[" + i + "]=" + SA[i]);
                    }

                    return -2;
                }
            }

            /* check first characters. */
            for (i = 1; i < n; ++i)
            {
                if ((T[SA[i - 1]] & 0xFF) > (T[SA[i]] & 0xFF))
                {
                    if (verbose)
                    {
                        System.out.println("Suffixes in wrong order.");
                        System.out.println("  T[SA[" + (i - 1) + "]=" + SA[i - 1] + "]=" + (T[SA[i - 1]] & 0xFF));
                        System.out.println(" > T[SA[" + i + "]=" + SA[i] + "]=" + (T[SA[i]] & 0xFF));
                    }

                    return -3;
                }
            }

            /* check suffixes. */
            for (i = 0; i < 256; ++i)
                C[i] = 0;
            for (i = 0; i < n; ++i)
                ++C[T[i]& 0xFF];
            for (i = 0, p = 0; i < 256; ++i)
            {
                t = C[i];
                C[i] = p;
                p += t;
            }

            q = C[T[n - 1]& 0xFF];
            C[T[n - 1]] += 1;
            for (i = 0; i < n; ++i)
            {
                p = SA[i];

                if (0 < p)
                {
                    c = T[--p]&0xFF;
                    t = C[c];
                }
                else
                {
                    c = T[p = n - 1]&0xFF;
                    t = q;
                }

                if ((t < 0) || (p != SA[t]))
                {
                    if (verbose)
                    {
                        System.out.println("Suffixes in wrong position.");
                        System.out.println("  SA[" + t + "]=" + ((0 <= t) ? SA[t] : -1) + " or");
                        System.out.println("  SA[" + i + "]=" + SA[i]);
                    }

                    return -4;
                }

                if (t != q)
                {
                    ++C[c];
                    if ((n <= C[c]) || ((T[SA[C[c]]] & 0xFF)!= c))
                        C[c] = -1;
                }
            }

            C = null;

            if (verbose)
                System.out.println("Done.");

            return 0;
        }
    }
}
