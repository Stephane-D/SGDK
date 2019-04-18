package sgdk.rescomp.resource.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

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

    public void outCollision(CollisionBase c, ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH,
            boolean forceExport) throws IOException
    {
        if (c instanceof Box)
        {
            final Box cbox = (Box) c;

            if ((cbox.x < -128) || (cbox.x > 127) || (cbox.y < -128) || (cbox.y > 127))
                System.err.println("Error: collision '" + id + "' position X/Y is out of range (< -128 or > 127) !");
            if ((cbox.w > 255) || (cbox.h > 255))
                System.err.println("Error: collision '" + id + "' size W/H is out of range (> 255) !");

            outS.println("    dc.w    " + ((cbox.x << 8) | ((cbox.y << 0) & 0xFF)));
            outS.println("    dc.w    " + ((cbox.w << 8) | ((cbox.h << 0) & 0xFF)));
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

            outS.println("    dc.w    " + ((ccircle.x << 8) | ((ccircle.y << 0) & 0xFF)));
            outS.println("    dc.w    " + ccircle.ray);
            outB.write(ccircle.x);
            outB.write(ccircle.y);
            Util.outB(outB, ccircle.ray);
        }
        else
        {
            if (forceExport)
            {
                // fill with 0 values
                outS.println("    dc.w    " + 0);
                outS.println("    dc.w    " + 0);
                outB.write(0);
                outB.write(0);
                outB.write(0);
                outB.write(0);
            }
        }
    }

    @Override
    public void out(ByteArrayOutputStream outB, PrintWriter outS, PrintWriter outH) throws IOException
    {
        // Collision structure
        Util.decl(outS, outH, "Collision", id, 2, global);

        // collision types
        outS.println("    dc.w    " + ((typeHit.ordinal() << 8) | ((typeAttack.ordinal() << 0) & 0xFF)));
        // binary write
        outB.write(typeHit.ordinal());
        outB.write(typeAttack.ordinal());

        // collision itself
        outCollision(hit, outB, outS, outH, typeAttack != CollisionType.NONE);
        outCollision(attack, outB, outS, outH, false);

        outS.println();
    }
}