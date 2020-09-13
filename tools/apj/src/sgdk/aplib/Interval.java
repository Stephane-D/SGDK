package sgdk.aplib;

import java.util.ArrayList;
import java.util.List;

class Interval
{
    public List<Interval> children;
    public Interval parent;
    public int left;
    public int right;
    public int length;

    public Interval(int length, int left, Interval interval)
    {
        this.length = length;
        this.left = left;
        right = -1;
        children = new ArrayList<Interval>();

        if (interval != null)
            children.add(interval);
    }

    public Interval(Interval interval, int right)
    {
        length = interval.length;
        left = interval.left;
        this.right = right;

        children = interval.children;
        for (Interval cinterval : children)
            cinterval.parent = this;
    }

    public void add(Interval interval)
    {
        interval.parent = this;
        children.add(interval);
    }

    public List<Integer> indexes()
    {
        final List<Integer> result = new ArrayList<>();

        int l = left;
        int ci = 0;

        while (ci < children.size())
        {
            while (l < children.get(ci).left)
                result.add(l++);

            l = children.get(ci).right + 1;
            ci++;
        }

        while (l <= right)
            result.add(l++);

        return result;
    }
}
