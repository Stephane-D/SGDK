package sgdk.rescomp.resource.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import sgdk.rescomp.Resource;
import sgdk.rescomp.tool.Util;
import sgdk.rescomp.type.Basics.Box;
import sgdk.rescomp.type.Basics.Circle;
import sgdk.rescomp.type.Basics.CollisionBase;
import sgdk.rescomp.type.Basics.CollisionType;

public class Collision extends Resource
{
    public final CollisionType typeHit;
    public final CollisionType typeAttack;
    public final CollisionBase hit;
    public final CollisionBase attack;

    final int hc;

    public Collision(String id, CollisionBase collision)
    {
        super(id);

        if (collision instanceof Box)
            typeHit = CollisionType.BOX;
        else if (collision instanceof Circle)
            typeHit = CollisionType.CIRCLE;
        else
            typeHit = CollisionType.NONE;
        hit = collision;
        typeAttack = CollisionType.NONE;
        attack = null;

        // compute hash code
        hc = typeHit.hashCode() ^ ((typeHit != CollisionType.NONE) ? hit.hashCode() : 0) ^ (typeAttack.hashCode() << 16)
                ^ ((typeAttack != CollisionType.NONE) ? attack.hashCode() << 16 : 0);
    }

    @Override
    public int internalHashCode()
    {
        return hc;
    }

    @Override
    public boolean internalEquals(Object obj)
    {
        if (obj instanceof Collision)
        {
            final Collision collision = (Collision) obj;
            return (typeHit == collision.typeHit) && (typeAttack == collision.typeAttack)
                    && ((typeHit != CollisionType.NONE) ? typeHit.equals(collision.typeHit) : true)
                    && ((typeAttack != CollisionType.NONE) ? typeAttack.equals(collision.typeAttack) : true);
        }

        return false;
    }

    public void outCollision(CollisionBase c, ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH,
            boolean forceExport)
    {
        if (c instanceof Box)
        {
            final Box cbox = (Box) c;

            if ((cbox.x < -128) || (cbox.x > 127) || (cbox.y < -128) || (cbox.y > 127))
                System.err.println("Error: collision '" + id + "' position X/Y is out of range (< -128 or > 127) !");
            if ((cbox.w > 255) || (cbox.h > 255))
                System.err.println("Error: collision '" + id + "' size W/H is out of range (> 255) !");

            outS.append("    dc.w    " + ((cbox.x << 8) | ((cbox.y << 0) & 0xFF)) + "\n");
            outS.append("    dc.w    " + ((cbox.w << 8) | ((cbox.h << 0) & 0xFF)) + "\n");
            outB.write(cbox.x);
            outB.write(cbox.y);
            outB.write(cbox.w);
            outB.write(cbox.h);
        }
        else if (c instanceof Circle)
        {
            final Circle ccircle = (Circle) c;

            if ((ccircle.x < -128) || (ccircle.x > 127) || (ccircle.y < -128) || (ccircle.y > 127))
                System.err.println("Error: collision '" + id + "' position X/Y is out of range (< -128 or > 127) !");

            outS.append("    dc.w    " + ((ccircle.x << 8) | ((ccircle.y << 0) & 0xFF)) + "\n");
            outS.append("    dc.w    " + ccircle.ray + "\n");
            outB.write(ccircle.x);
            outB.write(ccircle.y);
            Util.outB(outB, (short) ccircle.ray);
        }
        else
        {
            if (forceExport)
            {
                // fill with 0 values
                outS.append("    dc.w    " + 0 + "\n");
                outS.append("    dc.w    " + 0 + "\n");
                outB.write(0);
                outB.write(0);
                outB.write(0);
                outB.write(0);
            }
        }
    }

    @Override
    public int shallowSize()
    {
        return 2 + 1 + 1
                + ((typeAttack != CollisionType.NONE) ? 8 : ((hit != null) ? 4 : 0) + ((attack != null) ? 4 : 0));
    }

    @Override
    public int totalSize()
    {
        return shallowSize();
    }

    @Override
    public void out(ByteArrayOutputStream outB, StringBuilder outS, StringBuilder outH) throws IOException
    {
        // Collision structure
        Util.decl(outS, outH, "Collision", id, 2, global);

        // collision types
        outS.append("    dc.w    " + ((typeHit.ordinal() << 8) | ((typeAttack.ordinal() << 0) & 0xFF)) + "\n");
        // binary write
        outB.write(typeHit.ordinal());
        outB.write(typeAttack.ordinal());

        // collision itself
        outCollision(hit, outB, outS, outH, typeAttack != CollisionType.NONE);
        outCollision(attack, outB, outS, outH, false);

        outS.append("\n");
    }
}