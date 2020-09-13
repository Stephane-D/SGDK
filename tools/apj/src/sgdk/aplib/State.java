package sgdk.aplib;

class State
{
    public int bitcost;
    public int[] edge;
    public int index;
    public int steps;
    public int roffs;
    public State parent;

    public int lwm()
    {
        return (edge == null || edge[0] == 1 || edge[1] == 0) ? 0 : 1;
    }

    public State()
    {
        bitcost = 0;
        edge = null;
        index = 0;
        parent = null;
        roffs = 0;
        steps = 0;
    }

    public State(State state, int[] edge, byte[] input) throws Exception
    {
        bitcost = state.bitcost + Model.cost(state, edge, input);
        this.edge = edge;
        index = state.index + edge[0];
        parent = state;
        roffs = ((edge != null) && (edge[0] > 1)) ? edge[1] : state.roffs;
        steps = state.steps + 1;
    }

}
