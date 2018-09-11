package org.sgdk.resourcemanager.ui.utils.vgm;

import java.io.*;
import java.util.zip.*;
import java.net.*;

class DataReader {
	// Opens InputStream to file stored in various ways
	static InputStream openHttp(URL path) throws Exception {
		return path.openStream();
	}

	static InputStream openFile(String path) throws Exception {
		return new FileInputStream(new File(path));
	}

	static InputStream openGZIP(InputStream in) throws Exception {
		return new GZIPInputStream(in);
	}

	// "Resizes" array to new size and preserves elements from in
	static byte[] resize(byte[] in, int size) {
		byte[] out = new byte[size];
		if (size > in.length)
			size = in.length;
		System.arraycopy(in, 0, out, 0, size);
		return out;
	}

	// Loads entire stream into byte array, then closes stream
	static byte[] loadData(InputStream in) throws Exception {
		byte[] data = new byte[256 * 1024];
		int size = 0;
		int count;
		while ((count = in.read(data, size, data.length - size)) != -1) {
			size += count;
			if (size >= data.length)
				data = resize(data, data.length * 2);
		}
		in.close();

		if (data.length - size > data.length / 4)
			data = resize(data, size);

		return data;
	}

	// Loads stream into ByteArrayInputStream
	static ByteArrayInputStream cacheStream(InputStream in) throws Exception {
		return new ByteArrayInputStream(loadData(in));
	}

	// Finds file named 'path' inside zip file, or returns null if not found.
	// You should use a BufferedInputStream or cacheStream() for input.
	static InputStream openZip(InputStream in, String path) throws Exception {
		ZipInputStream zis = new ZipInputStream(in);
		for (ZipEntry entry; (entry = zis.getNextEntry()) != null;) {
			if (path.equals(entry.getName()))
				return zis;
		}
		return null;
	}
}