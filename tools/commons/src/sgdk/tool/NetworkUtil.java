package sgdk.tool;

import java.awt.Desktop;
import java.awt.Desktop.Action;
import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;
import java.util.Map;
import java.util.Map.Entry;

/**
 * @author stephane
 */
public class NetworkUtil
{

    /**
     * Open an URL in the default system browser
     */
    public static boolean openBrowser(String url)
    {
        return openBrowser(URLUtil.getURL(url));
    }

    /**
     * Open an URL in the default system browser
     */
    public static boolean openBrowser(URL url)
    {
        if (url == null)
            return false;

        try
        {
            return openBrowser(url.toURI());
        }
        catch (URISyntaxException e)
        {
            // use other method
            return systemOpenBrowser(url.toString());
        }
    }

    /**
     * Open an URL in the default system browser
     */
    public static boolean openBrowser(URI uri)
    {
        if (uri == null)
            return false;

        final Desktop desktop = SystemUtil.getDesktop();

        if ((desktop != null) && desktop.isSupported(Action.BROWSE))
        {
            try
            {
                desktop.browse(uri);
                return true;
            }
            catch (IOException e)
            {
                // ignore
            }
        }

        // not
        return systemOpenBrowser(uri.toString());
    }

    /**
     * Open an URL in the default system browser (low level method)
     */
    private static boolean systemOpenBrowser(String url)
    {
        if (StringUtil.isEmpty(url))
            return false;

        try
        {
            if (SystemUtil.isMac())
            {
                Class<?> fileMgr = Class.forName("com.apple.eio.FileManager");
                Method openURL = fileMgr.getDeclaredMethod("openURL", new Class[] {String.class});
                openURL.invoke(null, new Object[] {url});
            }
            else if (SystemUtil.isWindows())
                Runtime.getRuntime().exec("rundll32 url.dll,FileProtocolHandler " + url);
            else
            {
                // assume Unix or Linux
                String[] browsers = {"firefox", "opera", "konqueror", "epiphany", "mozilla", "netscape"};
                String browser = null;
                for (int count = 0; count < browsers.length && browser == null; count++)
                {
                    if (Runtime.getRuntime().exec("which " + browsers[count]).waitFor() == 0)
                        browser = browsers[count];
                }
                if (browser == null)
                    throw new Exception("Could not find web browser");

                Runtime.getRuntime().exec(new String[] {browser, url});
            }

            return true;
        }
        catch (Exception e)
        {
            System.err.println("Error while opening system browser :\n" + e.toString());
            return false;
        }
    }

    /**
     * Download data from specified URL string and return it as an array of byte
     */
    public static byte[] download(String path, boolean displayError)
    {
        final File file = new File(FileUtil.getGenericPath(path));

        // path define a file ?
        if (file.exists())
            return download(file, displayError);

        final URL url = URLUtil.getURL(path);

        // error while building URL ?
        if (url == null)
        {
            if (displayError)
                System.err.println("Can't download '" + path + "', incorrect path !");

            return null;
        }

        return download(url, displayError);
    }

    /**
     * Download data from specified URL and return it as an array of byte.<br>
     * It returns <code>null</code> if an error occurred.
     */
    public static byte[] download(URL url, boolean displayError)
    {
        // check if this is a file
        if ((url != null) && URLUtil.isFileURL(url))
        {
            try
            {
                return download(new File(url.toURI()), displayError);
            }
            catch (URISyntaxException e)
            {
                if (displayError)
                    System.err.println("Can't download from '" + url + "', incorrect path !");

                return null;
            }
        }

        // get connection object
        final URLConnection uc = openConnection(url, true, displayError);

        // error --> exit
        if (uc == null)
            return null;
        // can't connect --> exit
        if (!connect(uc, displayError))
            return null;

        // get input stream
        final InputStream ip = getInputStream(uc, displayError);

        // error --> exit
        if (ip == null)
            return null;

        try
        {
            final byte[] result = download(ip, uc.getContentLength());

            // we test response code for HTTP connection
            if (uc instanceof HttpURLConnection)
            {
                final HttpURLConnection huc = (HttpURLConnection) uc;

                // not ok ?
                if (huc.getResponseCode() != HttpURLConnection.HTTP_OK)
                {
                    if (displayError)
                    {
                        System.err.println("Error while downloading '" + huc.getURL() + "':");
                        System.err.println(huc.getResponseMessage());
                    }

                    return null;
                }
            }

            return result;
        }
        catch (Exception e)
        {
            if (displayError)
            {
                System.err.println("Error while downloading '" + uc.getURL() + "' :");
                System.err.println(e.getMessage());
            }

            return null;
        }
        finally
        {
            try
            {
                ip.close();
            }
            catch (IOException e)
            {
                // ignore...
            }
        }
    }

    /**
     * Download data from File and return it as an array of byte.<br>
     * It returns <code>null</code> if an error occurred (file not found or not existing, IO
     * error...)
     */
    public static byte[] download(File f, boolean displayError)
    {
        if (!f.exists())
        {
            System.err.println("File not found: " + f.getPath());
            return null;
        }

        try
        {
            return download(new FileInputStream(f), f.length());
        }
        catch (Exception e)
        {
            if (displayError)
            {
                System.err.println("NetworkUtil.download('" + f.getPath() + "',...) error :");
                System.err.println(e.getMessage());
            }

            return null;
        }
    }

    /**
     * Download data from specified InputStream and return it as an array of byte.<br>
     * Returns <code>null</code> if load operation was interrupted by user.
     */
    public static byte[] download(InputStream in, long len) throws IOException
    {
        final int READ_BLOCKSIZE = 64 * 1024;
        final BufferedInputStream bin;

        if (in instanceof BufferedInputStream)
            bin = (BufferedInputStream) in;
        else
            bin = new BufferedInputStream(in);

        final ByteArrayOutputStream bout = new ByteArrayOutputStream((int) ((len > 0) ? len : READ_BLOCKSIZE));
        // read per block of 64 KB
        final byte[] data = new byte[READ_BLOCKSIZE];

        try
        {
            int off = 0;
            int count = 0;

            while (count >= 0)
            {
                count = bin.read(data);
                if (count <= 0)
                {
                    // unexpected length
                    if ((len != -1) && (off != len))
                        throw new EOFException("Unexpected end of file at " + off + " (" + len + " expected)");
                }
                else
                    off += count;

                // copy to dynamic buffer
                if (count > 0)
                    bout.write(data, 0, count);
            }
        }
        finally
        {
            bin.close();
        }

        return bout.toByteArray();
    }

    /**
     * Download data from specified InputStream and return it as an array of byte.<br>
     * It returns <code>null</code> if an error occurred.
     */
    public static byte[] download(InputStream in) throws IOException
    {
        return download(in, -1);
    }

    /**
     * Returns a new {@link URLConnection} from specified URL (null if an error occurred).
     * 
     * @param url
     *        url to connect.
     * @param login
     *        login if the connection requires authentication.<br>
     *        Set it to null if no authentication needed.
     * @param pass
     *        login if the connection requires authentication.
     *        Set it to null if no authentication needed.
     * @param disableCache
     *        Disable proxy cache if any.
     * @param displayError
     *        Display error message in console if something wrong happen.
     */
    public static URLConnection openConnection(URL url, boolean disableCache, boolean displayError)
    {
        if (url == null)
        {
            if (displayError)
                System.out.println("NetworkUtil.openConnection(URL url, ...) error : URL is null !");

            return null;
        }

        try
        {
            final URLConnection uc = url.openConnection();

            if (disableCache)
                disableCache(uc);

            return uc;
        }
        catch (IOException e)
        {
            if (displayError)
            {
                System.out.println("NetworkUtil.openConnection('" + url + "',...) error :");
                System.err.println(e.getMessage());
            }

            return null;
        }
    }

    /**
     * Returns a new {@link URLConnection} from specified path.<br>
     * Returns <code>null</code> if an error occurred.
     * 
     * @param path
     *        path to connect.
     * @param disableCache
     *        Disable proxy cache if any.
     * @param displayError
     *        Display error message in console if something wrong happen.
     */
    public static URLConnection openConnection(String path, boolean disableCache, boolean displayError)
    {
        return openConnection(URLUtil.getURL(path), disableCache, displayError);
    }

    /**
     * Connect the specified {@link URLConnection}.<br>
     * Returns false if the connection failed or if response code is not ok.
     * 
     * @param uc
     *        URLConnection to connect.
     * @param displayError
     *        Display error message in console if something wrong happen.
     */
    public static boolean connect(URLConnection uc, boolean displayError)
    {
        try
        {
            final URL prevUrl = uc.getURL();

            // connect
            uc.connect();

            // we have to test that as sometime url are automatically modified / fixed by host!
            if (!uc.getURL().toString().toLowerCase().equals(prevUrl.toString().toLowerCase()))
            {
                // TODO : do something better
                System.out.println("Host URL change rejected : " + prevUrl + " --> " + uc.getURL());
                return false;
            }

            // we test response code for HTTP connection
            if (uc instanceof HttpURLConnection)
            {
                final HttpURLConnection huc = (HttpURLConnection) uc;

                // not ok ?
                if (huc.getResponseCode() != HttpURLConnection.HTTP_OK)
                {
                    if (displayError)
                    {
                        System.out.println("Error while connecting to '" + huc.getURL() + "':");
                        System.out.println(huc.getResponseMessage());
                    }

                    return false;
                }
            }
        }
        catch (Exception e)
        {
            if (displayError)
            {
                System.out.println("Error while connecting to '" + uc.getURL() + "':");
                System.err.println(e.getMessage());
            }

            return false;
        }

        return true;
    }

    /**
     * Returns a new {@link InputStream} from specified {@link URLConnection} (null if an error
     * occurred).
     * 
     * @param uc
     *        URLConnection object.
     * @param displayError
     *        Display error message in console if something wrong happen.
     */
    public static InputStream getInputStream(URLConnection uc, boolean displayError)
    {
        if (uc == null)
        {
            if (displayError)
            {
                System.out.print("NetworkUtil.getInputStream(URLConnection uc) error: ");
                System.out.println("URLConnection object is null !");
            }

            return null;
        }

        try
        {
            return uc.getInputStream();
        }
        catch (IOException e)
        {
            if (displayError)
            {
                System.out.println("Error while connecting to '" + uc.getURL() + "' :");
                System.err.println(e.getMessage());
            }

            return null;
        }
    }

    /**
     * Returns a new {@link InputStream} from specified URL (null if an error occurred).
     * 
     * @param url
     *        url we want to connect and retrieve the InputStream.
     * @param disableCache
     *        Disable proxy cache if any.
     * @param displayError
     *        Display error message in console if something wrong happen.
     */
    public static InputStream getInputStream(URL url, boolean disableCache, boolean displayError)
    {
        final URLConnection uc = openConnection(url, disableCache, displayError);

        if (uc != null)
            if (connect(uc, displayError))
                return getInputStream(uc, displayError);

        return null;

    }

    public static void disableCache(URLConnection uc)
    {
        uc.setDefaultUseCaches(false);
        uc.setUseCaches(false);
        uc.setRequestProperty("Cache-Control", "no-cache");
        uc.setRequestProperty("Pragma", "no-cache");
    }

    public static String getContentString(Map<String, String> values)
    {
        String result = "";

        for (Entry<String, String> entry : values.entrySet())
        {
            try
            {
                final String key = entry.getKey();

                if (!StringUtil.isEmpty(key))
                {
                    final String value = entry.getValue();

                    result += "&" + URLEncoder.encode(key, "UTF-8") + "=";

                    if (!StringUtil.isEmpty(value))
                        result += URLEncoder.encode(value, "UTF-8");
                }
            }
            catch (UnsupportedEncodingException e)
            {
                // no encoding
                result += "&" + entry.getKey() + "=" + entry.getValue();
            }
        }

        // remove the initial "&" character
        return result.substring(1);
    }

    public static String postData(String target, Map<String, String> values) throws IOException
    {
        return postData(target, getContentString(values));
    }

    public static String postData(String target, String content) throws IOException
    {
        String response = "";

        final URLConnection uc = openConnection(target, true, true);

        if (uc == null)
            return null;

        // set connection parameters
        uc.setDoInput(true);
        uc.setDoOutput(true);

        // make server believe we are form data...
        uc.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");

        final DataOutputStream out = new DataOutputStream(uc.getOutputStream());
        try
        {
            // write out the bytes of the content string to the stream
            out.writeBytes(content);
            out.flush();
        }
        finally
        {
            out.close();
        }

        // read response from the input stream.
        final InputStream inStream = getInputStream(uc, false);
        if (inStream == null)
            return null;

        final BufferedReader in = new BufferedReader(new InputStreamReader(inStream));

        try
        {
            String temp;
            while ((temp = in.readLine()) != null)
                response += temp + "\n";
        }
        finally
        {
            in.close();
        }

        return response;
    }

    public static void enableSystemProxy()
    {
        SystemUtil.setProperty("java.net.useSystemProxies", "true");
    }

    public static void disableSystemProxy()
    {
        SystemUtil.setProperty("java.net.useSystemProxies", "false");
    }

    public static void enableProxySetting()
    {
        SystemUtil.setProperty("proxySet", "true");
    }

    public static void disableProxySetting()
    {
        SystemUtil.setProperty("proxySet", "false");
    }

    public static void enableHTTPProxySetting()
    {
        SystemUtil.setProperty("http.proxySet", "true");
    }

    public static void disableHTTPProxySetting()
    {
        SystemUtil.setProperty("http.proxySet", "false");
    }

    public static void enableHTTPSProxySetting()
    {
        SystemUtil.setProperty("https.proxySet", "true");
    }

    public static void disableHTTPSProxySetting()
    {
        SystemUtil.setProperty("https.proxySet", "false");
    }

    public static void enableFTPProxySetting()
    {
        SystemUtil.setProperty("ftp.proxySet", "true");
    }

    public static void disableFTPProxySetting()
    {
        SystemUtil.setProperty("ftp.proxySet", "false");
    }

    public static void enableSOCKSProxySetting()
    {
        SystemUtil.setProperty("socksProxySet", "true");
    }

    public static void disableSOCKSProxySetting()
    {
        SystemUtil.setProperty("socksProxySet", "false");
    }

    public static void setProxyHost(String host)
    {
        SystemUtil.setProperty("proxy.server", host);
    }

    public static void setProxyPort(int port)
    {
        SystemUtil.setProperty("proxy.port", Integer.toString(port));
    }

    public static void setHTTPProxyHost(String host)
    {
        SystemUtil.setProperty("http.proxyHost", host);
    }

    public static void setHTTPProxyPort(int port)
    {
        SystemUtil.setProperty("http.proxyPort", Integer.toString(port));
    }

    public static void setHTTPProxyUser(String user)
    {
        SystemUtil.setProperty("http.proxyUser", user);
    }

    public static void setHTTPProxyPassword(String password)
    {
        SystemUtil.setProperty("http.proxyPassword", password);
    }

    public static void setHTTPSProxyHost(String host)
    {
        SystemUtil.setProperty("https.proxyHost", host);
    }

    public static void setHTTPSProxyPort(int port)
    {
        SystemUtil.setProperty("https.proxyPort", Integer.toString(port));
    }

    public static void setHTTPSProxyUser(String user)
    {
        SystemUtil.setProperty("https.proxyUser", user);
    }

    public static void setHTTPSProxyPassword(String password)
    {
        SystemUtil.setProperty("https.proxyPassword", password);
    }

    public static void setFTPProxyHost(String host)
    {
        SystemUtil.setProperty("ftp.proxyHost", host);
    }

    public static void setFTPProxyPort(int port)
    {
        SystemUtil.setProperty("ftp.proxyPort", Integer.toString(port));
    }

    public static void setFTPProxyUser(String user)
    {
        SystemUtil.setProperty("ftp.proxyUser", user);
    }

    public static void setFTPProxyPassword(String password)
    {
        SystemUtil.setProperty("ftp.proxyPassword", password);
    }

    public static void setSOCKSProxyHost(String host)
    {
        SystemUtil.setProperty("socksProxyHost", host);
    }

    public static void setSOCKSProxyPort(int port)
    {
        SystemUtil.setProperty("socksProxyPort", Integer.toString(port));
    }

    public static void setSOCKSProxyUser(String user)
    {
        SystemUtil.setProperty("socksProxyUser", user);
    }

    public static void setSOCKSProxyPassword(String password)
    {
        SystemUtil.setProperty("socksProxyPassword", password);
    }

    public static String getProxyHost()
    {
        return SystemUtil.getProperty("proxy.server");
    }

    public static int getProxyPort()
    {
        try
        {
            return Integer.parseInt(SystemUtil.getProperty("proxy.port"));
        }
        catch (NumberFormatException e)
        {
            return 0;
        }
    }

    public static String getHTTPProxyHost()
    {
        return SystemUtil.getProperty("http.proxyHost");
    }

    public static int getHTTPProxyPort()
    {
        try
        {
            return Integer.parseInt(SystemUtil.getProperty("http.proxyPort"));
        }
        catch (NumberFormatException e)
        {
            return 0;
        }
    }

    public static String getHTTPSProxyHost()
    {
        return SystemUtil.getProperty("https.proxyHost");
    }

    public static int getHTTPSProxyPort()
    {
        try
        {
            return Integer.parseInt(SystemUtil.getProperty("https.proxyPort"));
        }
        catch (NumberFormatException e)
        {
            return 0;
        }
    }

    public static String getFTPProxyHost()
    {
        return SystemUtil.getProperty("ftp.proxyHost");
    }

    public static int getFTPProxyPort()
    {
        try
        {
            return Integer.parseInt(SystemUtil.getProperty("ftp.proxyPort"));
        }
        catch (NumberFormatException e)
        {
            return 0;
        }
    }

    public static String getSOCKSProxyHost()
    {
        return SystemUtil.getProperty("socksProxyHost");
    }

    public static int getSOCKSProxyPort()
    {
        try
        {
            return Integer.parseInt(SystemUtil.getProperty("socksProxyPort"));
        }
        catch (NumberFormatException e)
        {
            return 0;
        }
    }
}
