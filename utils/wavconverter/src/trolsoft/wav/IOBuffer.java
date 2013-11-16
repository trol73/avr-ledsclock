package trolsoft.wav;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class IOBuffer {
	

	/**
	 * Local buffer used for IO
	 */
	private byte[] buffer;
	
	/**
	 * Points to the current position in local buffer
	 */
	private int bufferPointer; 
	
	/**
	 * Bytes read after last read into local buffer
	 */
	private int bytesRead;
	
	/**
	 * 
	 */
	private int size;
	
	
	/**
	 * 
	 * @param size
	 */
	public IOBuffer(int size) {
		this.size = size;
		buffer = new byte[size];
		reset();
	}
	
	
	public void reset() {
		bufferPointer = 0;
		bytesRead = 0;
	}
	
	/**
	 * Get and Put little endian data from local buffer
	 * 
	 * @param pos
	 * @param numBytes
	 * @return
	 */
	public long getLE(int pos, int numBytes) {
		numBytes--;
		pos += numBytes;

		long val = buffer[pos] & 0xFF;
		for (int b = 0; b < numBytes; b++) {
			val = (val << 8) + (buffer[--pos] & 0xFF);
		}

		return val;
	}

	/**
	 * 
	 * @param val
	 * @param pos
	 * @param numBytes
	 */
	public void putLE(long val, int pos, int numBytes) {
		for (int b = 0; b < numBytes; b++) {
			buffer[pos] = (byte) (val & 0xFF);
			val >>= 8;
			pos++;
		}
	}
	
	/**
	 * 
	 * @param os
	 * @param off
	 * @param len
	 * @throws IOException 
	 */
	public void write(OutputStream os, int off, int len) throws IOException {
		os.write(buffer, off, len);
	}
	
	
	/**
	 * 
	 * @param os
	 * @throws IOException 
	 */
	public void write(OutputStream os) throws IOException {
		if (bufferPointer > 0) {
			os.write(buffer, 0, bufferPointer);
		}
	}	
	
	/**
	 * 
	 * @param is
	 * @param off
	 * @param len
	 * @throws IOException
	 * @return
	 */
	public int read(InputStream is, int off, int len) throws IOException {
		return is.read(buffer, off, len);
	}
	
	/**
	 * 
	 * @param os
	 * @param val
	 * @throws IOException
	 */
	public void write(OutputStream os, long val) throws IOException {
		if ( bufferPointer == size ) {
			os.write(buffer, 0, size);
			bufferPointer = 0;
		}

		buffer[bufferPointer] = (byte) (val & 0xFF);
		bufferPointer++;
	}
	
	
	public int read(InputStream is) throws IOException {
		if ( bufferPointer == bytesRead ) {
			int read = is.read(buffer, 0, size);
			if ( read == -1 ) {
				throw new IOException("Not enough data available");
			}
			bytesRead = read;
			bufferPointer = 0;
		}

		return buffer[bufferPointer++];
	}


}
