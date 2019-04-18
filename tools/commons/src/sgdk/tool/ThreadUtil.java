package sgdk.tool;

import java.awt.EventQueue;
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;

import javax.swing.SwingUtilities;

/**
 * Thread utilities class.
 * 
 * @author Stephane
 */
public class ThreadUtil
{
    /**
     * This class is used to catch exception in the EDT.
     */
    public static class CaughtRunnable implements Runnable
    {
        private final Runnable runnable;

        public CaughtRunnable(Runnable runnable)
        {
            super();

            this.runnable = runnable;
        }

        @Override
        public void run()
        {
            try
            {
                runnable.run();
            }
            catch (Throwable t)
            {
                System.err.println(t.getMessage());
                t.printStackTrace();
            }
        }
    }

    /**
     * @return true if the current thread is an AWT event dispatching thread.
     */
    public static boolean isEventDispatchThread()
    {
        return EventQueue.isDispatchThread();
    }

    /**
     * Invoke the specified <code>Runnable</code> on the AWT event dispatching thread.<br>
     * Any exception is automatically caught by Icy exception handler.
     * 
     * @param wait
     *        If set to true, the method wait until completion, in this case you have to take
     *        attention to not cause any dead lock.
     * @throws InterruptedException
     * @throws InvocationTargetException
     * @see #invokeLater(Runnable)
     * @see #invokeNow(Runnable)
     */
    public static void invoke(Runnable runnable, boolean wait) throws InvocationTargetException, InterruptedException
    {
        if (wait)
            invokeNow(runnable);
        else
            invokeLater(runnable);
    }

    /**
     * Invoke the specified <code>Runnable</code> on the AWT event dispatching thread now and wait
     * until completion.<br>
     * Any exception is automatically caught by Icy exception handler, if you want to catch them use
     * {@link #invokeNow(Callable)} instead.<br>
     * Use this method carefully as it may lead to dead lock.
     * 
     * @throws InterruptedException
     * @throws InvocationTargetException
     */
    public static void invokeNow(Runnable runnable) throws InvocationTargetException, InterruptedException
    {
        if (isEventDispatchThread())
            runnable.run();
        else
            EventQueue.invokeAndWait(runnable);
    }

    /**
     * Invoke the specified <code>Runnable</code> on the AWT event dispatching thread.<br>
     * If we already are on the EDT the <code>Runnable</code> is executed immediately else it will
     * be executed later.
     * 
     * @see #invokeLater(Runnable, boolean)
     */
    public static void invokeLater(Runnable runnable)
    {
        invokeLater(runnable, false);
    }

    /**
     * Invoke the specified <code>Runnable</code> on the AWT event dispatching thread.<br>
     * Depending the <code>forceLater</code> parameter the <code>Runnable</code> can be executed
     * immediately if we are on the EDT.
     * 
     * @param forceLater
     *        If <code>true</code> the <code>Runnable</code> is forced to execute later even if we
     *        are on the Swing EDT.
     */
    public static void invokeLater(Runnable runnable, boolean forceLater)
    {
        final Runnable r = new CaughtRunnable(runnable);

        if ((!forceLater) && isEventDispatchThread())
            r.run();
        else
            EventQueue.invokeLater(r);
    }

    /**
     * Invoke the specified <code>Callable</code> on the AWT event dispatching thread now and return
     * the result.<br>
     * The returned result can be <code>null</code> when a {@link Throwable} exception happen.<br>
     * Use this method carefully as it may lead to dead lock.
     * 
     * @throws InterruptedException
     *         if the current thread was interrupted while waiting
     * @throws Exception
     *         if the computation threw an exception
     */
    public static <T> T invokeNow(Callable<T> callable) throws Exception
    {
        if (SwingUtilities.isEventDispatchThread())
            return callable.call();

        final FutureTask<T> task = new FutureTask<>(callable);

        try
        {
            EventQueue.invokeAndWait(task);
        }
        catch (InvocationTargetException e)
        {
            if (e.getTargetException() instanceof Exception)
                throw (Exception) e.getTargetException();

            throw new Exception(e.getTargetException());
        }

        try
        {
            return task.get();
        }
        catch (ExecutionException e)
        {
            if (e.getCause() instanceof Exception)
                throw (Exception) e.getCause();

            throw new Exception(e.getCause());
        }
    }

    /**
     * Invoke the specified {@link Callable} on the AWT event dispatching thread.<br>
     * Depending the <code>forceLater</code> parameter the <code>Callable</code> can be executed
     * immediately if we are on the EDT.
     * 
     * @param forceLater
     *        If <code>true</code> the <code>Callable</code> is forced to execute later even if we
     *        are on the EDT.
     */
    public static <T> Future<T> invokeLater(Callable<T> callable, boolean forceLater)
    {
        final FutureTask<T> task = new FutureTask<>(callable);
        invokeLater(task, forceLater);
        return task;
    }

    /**
     * Same as {@link Thread#sleep(long)} except Exception is caught and ignored.
     */
    public static void sleep(long milli)
    {
        try
        {
            Thread.sleep(milli);
        }
        catch (InterruptedException e)
        {
            // have to interrupt the thread
            Thread.currentThread().interrupt();
        }
    }

    /**
     * Same as {@link Thread#sleep(long)} except Exception is caught and ignored.
     */
    public static void sleep(int milli)
    {
        try
        {
            Thread.sleep(milli);
        }
        catch (InterruptedException e)
        {
            // have to interrupt the thread
            Thread.currentThread().interrupt();
        }
    }
}
