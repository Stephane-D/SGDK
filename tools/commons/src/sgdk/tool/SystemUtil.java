package sgdk.tool;

import java.awt.Desktop;
import java.awt.Desktop.Action;
import java.awt.Event;
import java.awt.GraphicsEnvironment;
import java.awt.HeadlessException;
import java.awt.Toolkit;
import java.io.File;
import java.io.IOException;
import java.lang.ProcessBuilder.Redirect;
import java.util.Properties;

/**
 * @author stephane
 */
public class SystemUtil
{
    public static final String SYSTEM_WINDOWS = "win";
    public static final String SYSTEM_MAC_OS = "mac";
    public static final String SYSTEM_UNIX = "unix";

    /**
     * internals
     */
    private static Properties props = System.getProperties();

    /**
     * Launch specified jar file.
     * 
     * @param jarPath
     *        jar file path.
     * @param vmArgs
     *        arguments for the java virtual machine.
     * @param appArgs
     *        arguments for jar application.
     * @param workDir
     *        working directory.
     * @param redirectOuputs
     *        redirect out and err output stream from process into current process
     */
    public static Process execJAR(String jarPath, String vmArgs, String appArgs, String workDir, boolean redirectOuputs)
    {
        return exec(new String[] {"java", vmArgs, "-jar", jarPath, appArgs}, workDir, redirectOuputs);
    }

    /**
     * Launch specified jar file.
     * 
     * @param jarPath
     *        jar file path.
     * @param vmArgs
     *        arguments for the java virtual machine.
     * @param appArgs
     *        arguments for jar application.
     * @param redirectOuputs
     *        redirect out and err output stream from process into current process
     */
    public static Process execJAR(String jarPath, String vmArgs, String appArgs, boolean redirectOuputs)
    {
        return exec(new String[] {"java", vmArgs, "-jar", jarPath, appArgs}, redirectOuputs);
    }

    /**
     * Launch specified jar file.
     * 
     * @param jarPath
     *        jar file path.
     * @param appArgs
     *        arguments for jar application.
     */
    public static Process execJAR(String jarPath, String appArgs)
    {
        return execJAR(jarPath, "", appArgs, false);
    }

    /**
     * Launch specified jar file.
     * 
     * @param jarPath
     *        jar file path.
     */
    public static Process execJAR(String jarPath)
    {
        return execJAR(jarPath, "", "", false);
    }

    /**
     * Execute a system command and return the attached process.
     * 
     * @param cmd
     *        system command to execute.
     */
    public static Process exec(String[] cmd, boolean redirectOuputs)
    {
        return exec(cmd, ".", redirectOuputs);
    }

    /**
     * Execute a system command and return the attached process.
     * 
     * @param cmd
     *        system command to execute.
     * @param dir
     *        the working directory of the subprocess, or null if the subprocess should inherit the
     *        working directory of the current process.
     */
    public static Process exec(String[] cmd, String dir, boolean redirectOutput)
    {
        try
        {
            final ProcessBuilder builder = new ProcessBuilder(cmd);

            builder.directory(new File(dir));
            if (redirectOutput)
            {
                builder.redirectError(Redirect.INHERIT);
                builder.redirectOutput(Redirect.INHERIT);
            }

            return builder.start();
        }
        catch (Exception e)
        {
            System.err.println("SystemUtil.exec(" + cmd + ") error :");
            System.err.println(e.getMessage());
            return null;
        }
    }

    public static Desktop getDesktop()
    {
        if (Desktop.isDesktopSupported())
            return Desktop.getDesktop();

        return null;
    }

    /**
     * Launch the system file manager on specified folder (if supported)
     * 
     * @throws IOException
     */
    public static boolean openFolder(String folder) throws IOException
    {
        final Desktop desktop = getDesktop();

        if ((desktop != null) && desktop.isSupported(Action.OPEN))
        {
            desktop.open(new File(folder));
            return true;
        }

        return false;
    }

    /**
     * @see System#getProperty(String)
     */
    public static String getProperty(String name)
    {
        return props.getProperty(name);
    }

    /**
     * @see System#getProperty(String, String)
     */
    public static String getProperty(String name, String defaultValue)
    {
        return props.getProperty(name, defaultValue);
    }

    /**
     * @see System#setProperty(String, String)
     */
    public static String setProperty(String name, String value)
    {
        return (String) props.setProperty(name, value);
    }

    /**
     * Return the CTRL key mask used for Menu shortcut.
     */
    public static int getMenuCtrlMask()
    {
        try
        {
            return Toolkit.getDefaultToolkit().getMenuShortcutKeyMask();
        }
        catch (HeadlessException e)
        {
            // headless mode, use default Ctrl Mask
            return Event.CTRL_MASK;
        }
    }

    /**
     * Returns true if current system is "head less" (no screen output device).
     */
    public static boolean isHeadLess()
    {
        return GraphicsEnvironment.isHeadless();
    }

    public static ClassLoader getContextClassLoader()
    {
        return Thread.currentThread().getContextClassLoader();
    }

    public static ClassLoader getSystemClassLoader()
    {
        return ClassLoader.getSystemClassLoader();
    }

    /**
     * Return total number of processors or cores available to the JVM (same as system)
     */
    public static int getNumberOfCPUs()
    {
        return Runtime.getRuntime().availableProcessors();
    }

    /**
     * Return total amount of free memory available to the JVM (in bytes)
     */
    public static long getJavaFreeMemory()
    {
        return getJavaMaxMemory() - getJavaUsedMemory();
    }

    /**
     * Return maximum amount of memory the JVM will attempt to use (in bytes)
     */
    public static long getJavaMaxMemory()
    {
        return Runtime.getRuntime().maxMemory();
    }

    /**
     * Return memory currently allocated by the JVM (in bytes)
     */
    public static long getJavaAllocatedMemory()
    {
        return Runtime.getRuntime().totalMemory();
    }

    /**
     * Return actual memory used by the JVM (in bytes)
     */
    public static long getJavaUsedMemory()
    {
        return getJavaAllocatedMemory() - Runtime.getRuntime().freeMemory();
    }

    /**
     * Return total physic memory of system (in bytes)
     */
    public static long getTotalMemory()
    {
        return Runtime.getRuntime().totalMemory();
    }

    /**
     * Return free physic memory of system (in bytes)
     */
    public static long getFreeMemory()
    {
        return Runtime.getRuntime().freeMemory();
    }

    /**
     * Returns the user name.
     */
    public static String getUserName()
    {
        return getProperty("user.name");
    }

    /**
     * Returns the JVM name.
     */
    public static String getJavaName()
    {
        return getProperty("java.runtime.name");
    }

    /**
     * Returns the JVM version.
     */
    public static String getJavaVersion()
    {
        String result = getProperty("java.runtime.version");

        if (result.equals("unknow"))
            result = getProperty("java.version");

        return result;
    }

    /**
     * Returns the JVM version in number format (ex: 1.6091)
     */
    public static double getJavaVersionAsNumber()
    {
        String version = getJavaVersion().replaceAll("[^\\d.]", "");
        final int firstSepInd = version.indexOf('.');

        if (firstSepInd >= 0)
        {
            int lastSepInd = version.lastIndexOf('.');
            while (lastSepInd != firstSepInd)
            {
                version = version.substring(0, lastSepInd) + version.substring(lastSepInd + 1);
                lastSepInd = version.lastIndexOf('.');
            }
        }

        return StringUtil.parseDouble(version, 0);
    }

    /**
     * Returns the JVM data architecture model.
     */
    public static int getJavaArchDataModel()
    {
        return Integer.parseInt(getProperty("sun.arch.data.model"));
    }

    /**
     * Returns the Operating System name.
     */
    public static String getOSName()
    {
        return getProperty("os.name");
    }

    /**
     * Returns the Operating System architecture name.
     */
    public static String getOSArch()
    {
        return getProperty("os.arch");
    }

    /**
     * Returns the Operating System version.
     */
    public static String getOSVersion()
    {
        return getProperty("os.version");
    }

    /**
     * Return an id OS string :<br>
     * <br>
     * Windows system return <code>SystemUtil.SYSTEM_WINDOWS</code><br>
     * MAC OS return <code>SystemUtil.SYSTEM_MAC_OS</code><br>
     * Unix system return <code>SystemUtil.SYSTEM_UNIX</code><br>
     * <br>
     * An empty string is returned is OS is unknown.
     */
    public static String getOSNameId()
    {
        if (isWindows())
            return SYSTEM_WINDOWS;
        if (isMac())
            return SYSTEM_MAC_OS;
        if (isUnix())
            return SYSTEM_UNIX;

        return "";
    }

    /**
     * Return an id OS architecture string<br>
     * example : "win32", "win64", "mac32", "mac64", "unix32"...<br>
     * The bits number depends only from current installed JVM (32 or 64 bit)
     * and not directly from host OS.<br>
     * An empty string is returned if OS is unknown.
     */
    public static String getOSArchIdString()
    {
        final String javaBit = Integer.toString(getJavaArchDataModel());

        if (isWindows())
            return SYSTEM_WINDOWS + javaBit;
        if (isMac())
            return SYSTEM_MAC_OS + javaBit;
        if (isUnix())
            return SYSTEM_UNIX + javaBit;

        return "";
    }

    /**
     * Returns true is the operating system support link (symbolic or not).
     */
    public static boolean isLinkSupported()
    {
        return isMac() || isUnix();
    }

    /**
     * Returns true is the JVM is 32 bits.
     */
    public static boolean is32bits()
    {
        return getJavaArchDataModel() == 32;
    }

    /**
     * Returns true is the JVM is 64 bits.
     */
    public static boolean is64bits()
    {
        return getJavaArchDataModel() == 64;
    }

    /**
     * Returns true is the Operating System is Windows based.
     */
    public static boolean isWindows()
    {
        return (getOSName().toLowerCase().indexOf("win") >= 0);
    }

    /**
     * Returns true is the Operating System is Mac OS based.
     */
    public static boolean isMac()
    {
        return (getOSName().toLowerCase().indexOf("mac") >= 0);
    }

    /**
     * Returns true is the Operating System is Unix / Linux based.
     */
    public static boolean isUnix()
    {
        final String os = getOSName().toLowerCase();
        return (os.indexOf("nix") >= 0) || (os.indexOf("nux") >= 0);
    }

    /**
     * Returns true is the Operating System is Windows 64 bits whatever is the JVM installed (32 or
     * 64 bits).
     */
    public static boolean isWindows64()
    {
        if (!isWindows())
            return false;

        final String wow64Arch = System.getenv("PROCESSOR_ARCHITEW6432");
        if ((wow64Arch != null) && wow64Arch.endsWith("64"))
            return true;

        final String arch = System.getenv("PROCESSOR_ARCHITECTURE");
        if ((arch != null) && arch.endsWith("64"))
            return true;

        return false;
    }

    /**
     * Returns default temporary directory.<br>
     * ex:<br>
     * <code>c:/temp</code><br>
     * <code>/tmp</code><br>
     * Same as {@link FileUtil#getTempDirectory()}
     */
    public static String getTempDirectory()
    {
        return FileUtil.getTempDirectory();
    }

    /**
     * Returns temporary native library path (used to load native libraries from plugin)
     */
    public static String getTempLibraryDirectory()
    {
        return FileUtil.getTempDirectory() + "/lib";
    }

    /**
     * Load the specified native library.
     * 
     * @param dir
     *        directory from where we want to load the native library.
     * @param name
     *        name of the library.<br/>
     *        The filename of the library is automatically built depending the operating system.
     */
    public static void loadLibrary(String dir, String name)
    {
        final File libPath = new File(dir, System.mapLibraryName(name));

        if (libPath.exists())
            System.load(libPath.getAbsolutePath());
        else
            System.loadLibrary(name);
    }

    /**
     * Load the specified native library.
     * 
     * @param pathname
     *        complete path or name of the library we want to load
     */
    public static void loadLibrary(String pathname)
    {
        final File file = new File(pathname);

        if (file.exists())
            System.load(file.getAbsolutePath());
        else
            System.loadLibrary(pathname);
    }
}