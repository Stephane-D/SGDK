package sgdk.tool;

import java.io.File;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * @author Stephane
 */
public class URLUtil
{
    public static final String PROTOCOL_FILE = "file";
    public static final String PROTOCOL_FTP = "ftp";
    public static final String PROTOCOL_GOPHER = "gopher";
    public static final String PROTOCOL_HTTP = "http";
    public static final String PROTOCOL_JAR = "jar";

    /**
     * Convert a network path to a URL
     */
    public static URL getURL(String path)
    {
        if (path == null)
            return null;

        // first we try as network URL
        try
        {
            return new URL(path);
        }
        catch (MalformedURLException e)
        {
            // then we try to get it as file URL
            try
            {
                return new File(path).toURI().toURL();
            }
            catch (MalformedURLException e2)
            {
                return null;
            }
        }
    }

    public static boolean isURL(String path)
    {
        return getURL(path) != null;
    }

    public static boolean isNetworkURL(String path)
    {
        return isNetworkURL(getURL(path));
    }

    public static boolean isNetworkURL(URL url)
    {
        return (url != null) && !url.getProtocol().equals(PROTOCOL_FILE);
    }

    public static boolean isFileURL(String path)
    {
        return isFileURL(getURL(path));
    }

    public static boolean isFileURL(URL url)
    {
        return (url != null) && url.getProtocol().equals(PROTOCOL_FILE);
    }

    /**
     * Returns <code>true</code> if the url defines an absolute address and <code>false</code> if it
     * defines a relative address.
     */
    public static boolean isAbsolute(String path)
    {
        if (!StringUtil.isEmpty(path))
        {
            int index = path.indexOf(':');

            // protocol or drive letter
            if (index != -1)
            {
                if ((index + 1) < path.length())
                    return (path.charAt(index + 1) == '/');

                return false;
            }

            return (path.charAt(0) == '/');
        }

        return false;
    }

    public static String getNetworkURLString(String base, String path)
    {
        if (StringUtil.isEmpty(base))
            return path;
        if (StringUtil.isEmpty(path))
            return base;

        if (isNetworkURL(path))
            return path;

        return base + path;
    }

    public static String getURLProtocol(URL url)
    {
        if (url == null)
            return null;

        return url.getProtocol();
    }

    public static String getURLHost(URL url)
    {
        if (url == null)
            return null;

        return url.getHost();
    }

    public static String getURLDirectory(String url)
    {
        return getURLDirectory(getURL(url));
    }

    public static String getURLDirectory(URL url)
    {
        if (url != null)
        {
            final String path = url.getPath();

            if (!StringUtil.isEmpty(path))
            {
                int index = path.lastIndexOf('/');
                if (index != -1)
                    return path.substring(0, index + 1);

                index = path.lastIndexOf(':');
                if (index != -1)
                    return path.substring(0, index + 1);
            }
        }

        return "";
    }

    public static String getURLFileName(String url)
    {
        return getURLFileName(getURL(url));
    }

    public static String getURLFileName(String url, boolean withExtension)
    {
        return getURLFileName(getURL(url), withExtension);
    }

    public static String getURLFileName(URL url)
    {
        return getURLFileName(url, true);
    }

    public static String getURLFileName(URL url, boolean withExtension)
    {
        if (url == null)
            return "";

        final String path = url.getPath();

        if (StringUtil.isEmpty(path))
            return "";

        int index = path.lastIndexOf('/');
        final String fileName;

        if (index != -1)
            fileName = path.substring(index + 1);
        else
        {
            index = path.lastIndexOf(':');

            if (index != -1)
                fileName = path.substring(index + 1);
            else
                fileName = path;
        }

        if (withExtension)
            return fileName;

        index = fileName.lastIndexOf('.');

        if (index == 0)
            return "";
        else if (index != -1)
            return fileName.substring(0, index);
        else
            return fileName;
    }

    public static String getURLFileExtension(String url, boolean withDot)
    {
        return getURLFileExtension(getURL(url), withDot);
    }

    public static String getURLFileExtension(URL url, boolean withDot)
    {
        if (url == null)
            return "";

        final String path = url.getPath();

        if (StringUtil.isEmpty(path))
            return "";

        final int index = path.lastIndexOf('.');

        if (index == -1)
            return "";

        if (withDot)
            return path.substring(index);

        return path.substring(index + 1);
    }

    public static String getURLQuery(URL url)
    {
        if (url == null)
            return null;

        return url.getQuery();
    }

    /**
     * Build a URL from a base path and specified url.<br>
     * If the url is a relative address then result is the concatenation of base path and url.<br>
     * If the specified url is an absolute address then the url is returned as it is.
     */
    public static URL buildURL(String basePath, String url)
    {
        if (!isAbsolute(url) && !StringUtil.isEmpty(basePath))
            return getURL(basePath + url);

        return getURL(url);
    }
}
