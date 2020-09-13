package sgdk.aplib;

import sgdk.aplib.Esa.BEdge;

class Path
{
    private byte[] input;
    private Esa esa;

    public Path(byte[] input) throws Exception
    {
        this.input = input;
        esa = new Esa(input);
    }

    private State cheapest_of(State c1, State c2)
    {
        if (c1 == null || c2.bitcost < c1.bitcost || c2.bitcost == c1.bitcost && c2.steps < c1.steps)
            return c2;

        return c1;
    }

    private void evaluate_mo(State[] c_s, int index, int length, int offset) throws Exception
    {
        if (Model.valid(c_s[index], length, offset))
        {
            int dindex = index + length;
            if (c_s[dindex] == null || c_s[index].bitcost < c_s[dindex].bitcost)
            {
                State dstate = new State(c_s[index], new int[] {length, offset}, input);
                c_s[dindex] = cheapest_of(c_s[dindex], dstate);
            }
        }
    }

    private void evaluate_ro(State[] c_s, int[][] r_c, State[] r_s, int index, int length, int offset) throws Exception
    {
        if (r_s[offset] == null && r_c[offset] == null)
            evaluate_ro_rin_candidate(index, length, offset, r_c);
        else if (r_s[offset] != null && r_c[offset] == null)
        {
            int gindex = r_s[offset].index;
            int glength = index - gindex;

            if (glength <= Constant.threshold_gap_length)
            {
                State dstate = evaluate_ro_gapstate(r_s[offset], glength, c_s);

                if (dstate.index == index)
                    evaluate_ro_rmidrout(c_s, r_c, r_s, dstate, index, length, offset);
                else
                {
                    evaluate_ro_rin_candidate(index, length, offset, r_c);
                    r_s[offset] = null;
                }
            }
            else
            {
                evaluate_ro_rin_candidate(index, length, offset, r_c);
                r_s[offset] = null;
            }
        }
        else if (r_s[offset] == null && r_c[offset] != null)
        {
            int gindex = r_c[offset][0] + r_c[offset][1];
            int glength = index - gindex;

            if (glength <= Constant.threshold_gap_length)
            {
                State dstate = evaluate_ro_cheapest_rin(c_s, r_c[offset][0], r_c[offset][1], offset);

                if (c_s[dstate.index] == null || dstate.bitcost <= c_s[dstate.index].bitcost
                        + Constant.repeat_offset_edge_bitcost_maxdeviation)
                {
                    dstate = evaluate_ro_gapstate(dstate, glength, c_s);

                    if (dstate.index == index)
                        evaluate_ro_rmidrout(c_s, r_c, r_s, dstate, index, length, offset);
                    else
                        evaluate_ro_rin_candidate(index, length, offset, r_c);
                }
            }
            else
                evaluate_ro_rin_candidate(index, length, offset, r_c);
        }
        else if (r_s[offset] != null && r_c[offset] != null)
            throw new Exception("!WAT!");
        else
            throw new Exception("!WAT!");
    }

    private void evaluate_ro_rin_candidate(int index, int length, int offset, int[][] r_c)
    {
        if (Model.valid(length, offset))
            r_c[offset] = new int[] {index, length};
        else
            r_c[offset] = null;
    }

    private State evaluate_ro_cheapest_rin(State[] c_s, int index, int length, int offset) throws Exception
    {
        State state = null;

        while (length > 1)
        {
            if (c_s[index] != null && Model.valid(c_s[index], length, offset))
                state = cheapest_of(state, new State(c_s[index], new int[] {length, offset}, input));

            index++;
            length--;
        }

        return state;
    }

    private State evaluate_ro_gapstate(State state, int length, State[] c_s) throws Exception
    {
        int dindex = state.index + length;

        while (state.index < dindex)
        {
            state = new State(state, new int[] {1, esa.l1o[state.index]}, input);

            if (c_s[state.index] != null
                    && state.bitcost > c_s[state.index].bitcost + Constant.repeat_offset_gap_bitcost_maxdeviation)
                break;
        }

        return state;
    }

    private void evaluate_ro_rmidrout(State[] c_s, int[][] r_c, State[] r_s, State state, int index, int length,
            int offset) throws Exception
    {
        boolean do_r_s = true;

        int _length = length;

        while (length > 1)
        {
            State dstate = new State(state, new int[] {length, offset}, input);
            if (do_r_s)
            {
                if (Model.valid(length, offset))
                {
                    State cstate = new State(c_s[index], new int[] {length, offset}, input);
                    r_s[offset] = cheapest_of(cstate, dstate);
                }
                else
                    r_s[offset] = dstate;

                do_r_s = false;
            }
            c_s[dstate.index] = cheapest_of(c_s[dstate.index], dstate);

            length--;
        }

        length = _length;

        if (Model.valid(length, offset))
        {
            State rstate = evaluate_ro_cheapest_rin(c_s, index, length, offset);
            c_s[index + length] = cheapest_of(c_s[index + length], rstate);
            r_s[offset] = cheapest_of(r_s[offset], rstate);
        }

        r_c[offset] = null;
    }

    public int[][] output() throws Exception
    {
        State[] c_s = new State[esa.input.length + 1];
        c_s[0] = new State();

        int[][] r_c = new int[Constant.threshold_offset + 1][];
        State[] r_s = new State[Constant.threshold_offset + 1];

        for (int i = 0; i < input.length; i++)
        {
            if (c_s[i] != null)
            {
                for (BEdge t : esa.tedges(i))
                {
                    boolean maximal = t.maximal;
                    int length = t.length;
                    int offset = t.offset;

                    if (maximal)
                        evaluate_ro(c_s, r_c, r_s, i, length, offset);
                    else
                        evaluate_mo(c_s, i, length, offset);
                }
            }
            else
                throw new Exception("!WAT!");
        }

        State state = c_s[c_s.length - 1];
        int[][] output = new int[state.steps][];
        int op = output.length - 1;
        while (state.parent != null)
        {
            output[op--] = state.edge;
            state = state.parent;
        }

        if (!verify(input, output))
            throw new Exception("path b0rk");

        return output;
    }

    static private boolean verify(byte[] input, int[][] path)
    {
        int i = 0;

        for (int[] t : path)
        {
            int length = t[0];
            int offset = t[1];

            if (offset > 0)
            {
                int ci = i - offset;

                for (int l = 0; l < length; l++)
                    if (input[i + l] != input[ci + l])
                        return false;
            }

            i += length;
        }

        return true;
    }
}
