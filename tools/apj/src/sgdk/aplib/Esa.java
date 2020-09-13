package sgdk.aplib;

import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

public class Esa
{
    static class TupleIntervalInt
    {
        final Interval interval;
        final int value;

        public TupleIntervalInt(Interval i, int v)
        {
            super();

            interval = i;
            value = v;
        }
    }

    static class BEdge
    {
        final boolean maximal;
        final int length;
        final int offset;

        public BEdge(boolean maximal, int length, int offset)
        {
            super();

            this.maximal = maximal;
            this.length = length;
            this.offset = offset;
        }
    }

    public byte[] input;

    public int[] sa;
    public int[] isa;
    public int[] lcp;

    public int[] l1o;
    public Interval lit;
    public Interval[] litarray;

    public int[] snsv;
    public int[] spsv;
    public int[] snlv;
    public int[] splv;

    public Esa(byte[] input) throws Exception
    {
        this.input = input;

        sa = SuffixArray.SAIS.sa(input);
        isa = _isa(sa);
        lcp = _lcp(input, sa, isa);
        l1o = _l1o(input);
        lit = _lit(lcp);
        litarray = _litarray(lit);
        spsv = _psv(sa);
        snsv = _nsv(sa);
        splv = _plv(sa);
        snlv = _nlv(sa);
    }

    public static List<Interval> _dfs(Interval interval)
    {
        return _dfs(interval, null);
    }

    public static List<Interval> _dfs(Interval interval, Interval blocked)
    {
        final List<Interval> result = new ArrayList<>();
        final Stack<TupleIntervalInt> stack = new Stack<>();

        stack.push(new TupleIntervalInt(interval, 0));

        while (stack.size() > 0)
        {
            TupleIntervalInt si = stack.pop();

            if (si.interval != blocked)
            {
                if (si.value < si.interval.children.size())
                {
                    stack.push(new TupleIntervalInt(si.interval, si.value + 1));
                    stack.push(new TupleIntervalInt(si.interval.children.get(si.value), 0));
                }
                else
                    result.add(si.interval);
            }
        }

        return result;
    }

    static public int[] _isa(int[] sa)
    {
        int[] output = new int[sa.length];

        for (int i = 0; i < sa.length; i++)
            output[sa[i]] = i;

        return output;
    }

    static public int[] _l1o(byte[] input)
    {
        int[] output = new int[input.length];
        int[] t = new int[256];

        for (int i = 0; i < t.length; i++)
            t[i] = -1;

        for (int i = 0; i < input.length; i++)
        {
            int o = i - t[input[i] & 0xFF];

            if (input[i] == 0x00)
                output[i] = 0;
            else if (t[input[i] & 0xFF] > -1 && o < Constant.threshold_bytematch_offset)
                output[i] = o;
            else
                output[i] = 0;

            t[input[i] & 0xFF] = i;
        }

        return output;
    }

    // linear-time longest-common-prefix computation in suffix arrays and its applications - toru kasai 2001
    // http://alumni.cs.ucr.edu/~rakthant/cs234/01_KLAAP_Linear%20time%20LCP.PDF - page 6 "algorithm getheight"
    static public int[] _lcp(byte[] input, int[] sa, int[] isa)
    {
        int[] output = new int[input.length];

        int h = 0;
        for (int i = 0; i < input.length; i++)
        {
            int k = isa[i];
            if (k > 0)
            {
                int j = sa[k - 1];
                while (((i + h) < input.length) && ((j + h) < input.length) && (input[i + h] == input[j + h]))
                    h++;

                output[k] = h;
            }
            else
            {// k=0 undefined
                output[k] = -1;
            }
            if (h > 0)
                h--;
        }

        return output;
    }

    // lcp interval tree
    static private Interval _lit(int[] lcp)
    {
        Interval interval = null;

        Stack<Interval> stack = new Stack<>();
        stack.push(new Interval(0, 0, null));

        for (int i = 1; i < lcp.length; i++)
        {
            int left = i - 1;

            while (lcp[i] < stack.peek().length)
            {
                interval = new Interval(stack.pop(), i - 1);
                // process
                left = interval.left;
                if (lcp[i] <= stack.peek().length)
                {
                    stack.peek().add(interval);
                    interval = null;

                }
            }
            if (lcp[i] > stack.peek().length)
            {
                stack.push(new Interval(lcp[i], left, interval));
                interval = null;
            }
        }

        while (stack.size() > 1)
        {
            interval = stack.pop();
            interval = new Interval(interval, lcp.length - 1);
            // process
            if (interval.length > stack.peek().length)
            {
                stack.peek().add(interval);
            }
        }

        interval = new Interval(stack.pop(), lcp.length - 1);
        return interval;
    }

    static public Interval[] _litarray(Interval lit)
    {
        Interval[] output = new Interval[lit.right + 1];

        for (Interval interval : _dfs(lit))
            for (int n : interval.indexes())
                output[n] = interval;

        return output;
    }

    // computing longest previous factor in linear time and applications - maxime crochemore 20071024
    // https://hal.inria.fr/hal-00619691/document - page 6 "compute_lpf_using_lcp"
    static private int[] _lpf(int[] sa, int[] lcp)
    {
        int[] lpf = new int[sa.length];
        // var prevocc = new int[sa.length];

        Stack<int[]> stack = new Stack<>();// Item1 = pos, Item2 = len
        stack.push(new int[] {sa[0], 0});

        for (int i = 1; i <= lpf.length; i++)
        {
            int lcpi = i < lpf.length ? lcp[i] : 0;
            int sai = i < lpf.length ? sa[i] : -1;

            while (stack.size() > 0 && sai < stack.peek()[0])
            {
                int[] ti = stack.pop();
                lpf[ti[0]] = Math.max(ti[1], lcpi);
                lcpi = Math.min(ti[1], lcpi);

                /*
                 * if (lpf[ti[0]] == 0)
                 * {
                 * prevocc[ti[0]] = -1;
                 * }
                 * else if (ti[1] > lcpi)
                 * {
                 * prevocc[ti[0]] = stack.peek()[0];
                 * }
                 * else
                 * {
                 * prevocc[ti[0]] = sai;
                 * }
                 */
            }

            if (i < sa.length)
                stack.push(new int[] {sai, lcpi});
        }

        return lpf;
    }

    static public int[] _nlv(int[] input)
    {
        int[] nlv = new int[input.length];
        Stack<Integer> s = new Stack<Integer>();

        for (int i = 0; i < input.length; i++)
        {
            while (s.size() > 0 && input[s.peek()] < input[i])
            {
                nlv[s.peek()] = i;
                s.pop();
            }
            s.push(i);
        }

        while (s.size() > 0)
        {
            nlv[s.peek()] = -1;
            s.pop();
        }

        return nlv;
    }

    // simpler and faster lempel ziv factorization - keisuke goto & hideo bannai
    // https://pdfs.semanticscholar.org/87f4/f4a4f5a6508a20f356fab3ab97aa7062b0d5.pdf - page 7 algorithm 4
    static public int[] _nsv(int[] input)
    {
        int[] nsv = new int[input.length];
        Stack<Integer> s = new Stack<Integer>();

        for (int i = 0; i < input.length; i++)
        {
            int x = input[i];
            while (s.size() > 0 && input[s.peek()] > x)
            {
                nsv[s.peek()] = i;
                s.pop();
            }
            s.push(i);
        }

        while (s.size() > 0)
        {
            nsv[s.peek()] = -1;
            s.pop();
        }

        return nsv;
    }

    static public int[] _plv(int[] input)
    {
        int[] plv = new int[input.length];
        Stack<Integer> s = new Stack<Integer>();

        for (int i = input.length - 1; i >= 0; i--)
        {
            while (s.size() > 0 && input[s.peek()] < input[i])
            {
                plv[s.peek()] = i;
                s.pop();
            }
            s.push(i);
        }

        while (s.size() > 0)
        {
            plv[s.peek()] = -1;
            s.pop();
        }

        return plv;
    }

    static public int[] _psv(int[] input)
    {
        int[] psv = new int[input.length];
        Stack<Integer> s = new Stack<Integer>();

        for (int i = input.length - 1; i >= 0; i--)
        {
            while (s.size() > 0 && input[s.peek()] > input[i])
            {
                psv[s.peek()] = i;
                s.pop();
            }
            s.push(i);
        }

        while (s.size() > 0)
        {
            psv[s.peek()] = -1;
            s.pop();
        }

        return psv;
    }

    public List<BEdge> tedges(int i)
    {
        final List<BEdge> result = new ArrayList<>();
        Interval lcpinterval = litarray[isa[i]];
        int length = lcpinterval.length;

        int slp = spsv[isa[i]];
        int srp = snsv[isa[i]];

        int minoffset = Integer.MAX_VALUE;

        boolean this_rle = isrle(i);
        boolean former_rle = isrle(i - 1);
        boolean yieldminimaloffsetedges = true;

        while (lcpinterval.length > 1)
        {
            while (slp > -1 && slp >= lcpinterval.left)
            {
                int offset = i - sa[slp];
                minoffset = Math.min(minoffset, offset);
                int src = i - offset;
                boolean leftmaximal = src == 0 || input[i - 1] != input[src - 1];

                if (leftmaximal)
                    result.add(new BEdge(leftmaximal, lcpinterval.length, offset));

                if (splv[slp] > -1 && sa[splv[slp]] > i && (slp > 0 && spsv[slp] != -1 && splv[spsv[slp]] == spsv[slp]))
                    slp = spsv[slp];
                else if (spsv[slp] < 0 || slp - splv[slp] < slp - spsv[slp])
                {
                    slp = splv[slp];

                    while (slp > -1 && sa[slp] > i)
                        slp = spsv[slp];
                }
                else
                    slp = spsv[slp];
            }

            while (srp > -1 && srp <= lcpinterval.right)
            {
                int offset = i - sa[srp];
                minoffset = Math.min(minoffset, offset);
                int src = i - offset;
                boolean leftmaximal = src == 0 || input[i - 1] != input[src - 1];

                if (leftmaximal)
                    result.add(new BEdge(leftmaximal, lcpinterval.length, offset));

                if (snlv[srp] > -1 && sa[snlv[srp]] > i && (srp > 0 && snsv[srp] != -1 && snlv[snsv[srp]] == snsv[srp]))
                    srp = snsv[srp];
                else if (snsv[srp] < 0 || snlv[srp] - srp < snsv[srp] - srp)
                {
                    srp = snlv[srp];

                    while (srp > -1 && srp < sa.length && sa[srp] > i)
                        srp = snsv[srp];
                }
                else
                    srp = snsv[srp];
            }

            lcpinterval = lcpinterval.parent;

            if (minoffset < Integer.MAX_VALUE)
            {
                while (length > lcpinterval.length && yieldminimaloffsetedges)
                {
                    result.add(new BEdge(false, length, minoffset));
                    length--;
                    yieldminimaloffsetedges = !(this_rle && former_rle);
                }
            }
            else
                length = lcpinterval.length;

            if (length > Constant.threshold_greedy_length)
                break;
        }

        if (minoffset < Integer.MAX_VALUE)
        {
            while (length > 1 && yieldminimaloffsetedges)
            {
                result.add(new BEdge(false, length, minoffset));
                length--;
                if (length > Constant.threshold_greedy_length)
                    break;
            }
        }

        result.add(new BEdge(false, 1, l1o[i]));

        return result;
    }

    private boolean isrle(int i)
    {
        boolean output = false;
        if (i > 1)
        {
            Interval lcpinterval = litarray[isa[i]];
            output = i > 1 && isa[i] >= lcpinterval.left && isa[i - 1] >= lcpinterval.left
                    && isa[i] <= lcpinterval.right && isa[i - 1] <= lcpinterval.right;
        }
        return output;
    }

}