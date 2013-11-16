package trolsoft.wav;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.RandomAccessFile;




/**
 * Wav file IO class
 * A.Greensted
 * http://www.labbookpages.co.uk
 *
 * File format is based on the information from
 * http://www.sonicspot.com/guide/wavefiles.html
 * http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/wave.htm
 *
 * Version 1.0
 * 
 * @author trol
 *
 */
public class WavFile {
	
	
	
	private enum IOState {
		READING, WRITING, CLOSED
	};

	private final static int BUFFER_SIZE = 4096;

	private final static int FMT_CHUNK_ID = 0x20746D66;
	private final static int DATA_CHUNK_ID = 0x61746164;
	private final static int RIFF_CHUNK_ID = 0x46464952;
	private final static int RIFF_TYPE_ID = 0x45564157;

	
	/**
	 * Specifies the IO State of the Wav File (used for snaity checking)
	 */
	private IOState ioState;
	
	/**
	 * Number of bytes required to store a single sample
	 */
	private int bytesPerSample;
	
	/**
	 * Number of frames within the data section
	 */
	private long numFrames;
	
	/**
	 * Output stream used for writting data
	 */
	private OutputStream oStream;
	
	/**
	 * Input stream used for reading data	
	 */
	private InputStream iStream;
	
	/**
	 * Scaling factor used for int <-> float conversion
	 */
	private double floatScale;
	
	/**
	 * Offset factor used for int <-> float conversion
	 */
	private double floatOffset;
	
	/**
	 * Specify if an extra byte at the end of the data chunk is required for word alignment
	 */
	private boolean wordAlignAdjust;

	// Wav Header
	
	
	/**
	 * 2 bytes unsigned, 0x0001 (1) to 0xFFFF (65,535)
	 */
	private int numChannels;
	
	/**
	 * 4 bytes unsigned, 0x00000001 (1) to 0xFFFFFFFF (4,294,967,295)
	 * Although a java int is 4 bytes, it is signed, so need to use a long
	 */
	private long sampleRate;

	/**
	 * 2 bytes unsigned, 0x0001 (1) to 0xFFFF (65,535)
	 */
	private int blockAlign; 
	
	/**
	 * 2 bytes unsigned, 0x0002 (2) to 0xFFFF (65,535)
	 */
	private int validBits;

	/**
	 * Current number of frames read or written
	 */
	private long frameCounter;

	/**
	 * 
	 */
	private IOBuffer buffer;
	


	// Cannot instantiate WavFile directly, must either use newWavFile() or openWavFile()
	private WavFile() {
		buffer = new IOBuffer(BUFFER_SIZE);
	}

	
	/**
	 * Returns the number of channels.
	 * 
	 * @return
	 */
	public int getNumChannels() {
		return numChannels;
	}

	/**
	 * Returns the total number of frames stored in the wav file.
	 * 
	 * @return
	 */
	public long getNumFrames() {
		return numFrames;
	}
	
	/**
	 * 
	 * @param numFrames
	 */
	public void setNumFrames(long numFrames) {
		this.numFrames = numFrames;
	}

	/**
	 * Returns the remaining number of frames available for reading or writing.
	 * 
	 * @return
	 */
	public long getFramesRemaining() {
		return numFrames - frameCounter;
	}

	/**
	 * Returns the sample rate.
	 * 
	 * @return
	 */
	public long getSampleRate() {
		return sampleRate;
	}
	
	/**
	 * Returns the number of valid bits used for storing a single sample. This is the sample resolution.
	 * 
	 * @return
	 */
	public int getValidBits() {
		return validBits;
	}

	/**
	 * This method creates a new wav file for writing to. It writes header information to the wav file. 
	 * Once this method has been called and a WavFile instance retrieved, the writeFrames methods can be used 
	 * to write samples to the wav file.
	 * 
	 * @param file
	 * @param numChannels
	 * @param numFrames
	 * @param validBits
	 * @param sampleRate
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	
	public static WavFile newWavFile(File file, int numChannels, long numFrames, 
			int validBits, long sampleRate) throws IOException, WavFileException {
		return newWavFile(new FileOutputStream(file), numChannels, numFrames, validBits, sampleRate);		
	}
	
	/**
	 * 
	 * @param file
	 * @param numChannels
	 * @param numFrames
	 * @param validBits
	 * @param sampleRate
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static WavFile newWavFile(OutputStream os, int numChannels, long numFrames, 
			int validBits, long sampleRate) throws IOException, WavFileException {
		// Instantiate new Wavfile and initialise
		WavFile wavFile = new WavFile();
//		wavFile.file = file;
		wavFile.numChannels = numChannels;
		wavFile.numFrames = numFrames;
		wavFile.sampleRate = sampleRate;
		wavFile.bytesPerSample = (validBits + 7) / 8;
		wavFile.blockAlign = wavFile.bytesPerSample * numChannels;
		wavFile.validBits = validBits;

		// Sanity check arguments
		assertWav(numChannels >= 1 && numChannels <= 65535, "Illegal number of channels, valid range 1 to 65536");
		assertWav(numFrames >= 0, "Number of frames must be positive");
		assertWav(validBits >= 2 && validBits <= 65535, "Illegal number of valid bits, valid range 2 to 65536");
		assertWav(sampleRate >= 0, "Sample rate must be positive");

		// Create output stream for writing data
		wavFile.oStream = os;

		// Calculate the chunk sizes
		long dataChunkSize = wavFile.blockAlign * numFrames;
		long mainChunkSize = 4 + // Riff Type
				8 + // Format ID and size
				16 + // Format data
				8 + // Data ID and size
				dataChunkSize;

		// Chunks must be word aligned, so if odd number of audio data bytes
		// adjust the main chunk size
		if (dataChunkSize % 2 == 1) {
			mainChunkSize += 1;
			wavFile.wordAlignAdjust = true;
		} else {
			wavFile.wordAlignAdjust = false;
		}

		// Set the main chunk size
		wavFile.buffer.putLE(RIFF_CHUNK_ID, 0, 4);
		wavFile.buffer.putLE(mainChunkSize, 4, 4);
		wavFile.buffer.putLE(RIFF_TYPE_ID, 8, 4);

		// Write out the header
		wavFile.buffer.write(wavFile.oStream, 0, 12);

		// Put format data in buffer
		long averageBytesPerSecond = sampleRate * wavFile.blockAlign;

		wavFile.buffer.putLE(FMT_CHUNK_ID, 0, 4); // Chunk ID
		wavFile.buffer.putLE(16, 4, 4); // Chunk Data Size
		wavFile.buffer.putLE(1, 8, 2); // Compression Code (Uncompressed)
		wavFile.buffer.putLE(numChannels, 10, 2); // Number of channels
		wavFile.buffer.putLE(sampleRate, 12, 4); // Sample Rate
		wavFile.buffer.putLE(averageBytesPerSecond, 16, 4); // Average Bytes
																// Per Second
		wavFile.buffer.putLE(wavFile.blockAlign, 20, 2); // Block Align
		wavFile.buffer.putLE(validBits, 22, 2); // Valid Bits

		// Write Format Chunk
		wavFile.buffer.write(wavFile.oStream, 0, 24);

		// Start Data Chunk
		wavFile.buffer.putLE(DATA_CHUNK_ID, 0, 4); // Chunk ID
		wavFile.buffer.putLE(dataChunkSize, 4, 4); // Chunk Data Size

		// Write Format Chunk
		wavFile.buffer.write(wavFile.oStream, 0, 8);

		// Calculate the scaling factor for converting to a normalised double
		if ( wavFile.validBits > 8 ) {
			// If more than 8 validBits, data is signed
			// Conversion required multiplying by magnitude of max positive value
			wavFile.floatOffset = 0;
			wavFile.floatScale = Long.MAX_VALUE >> (64 - wavFile.validBits);
		} else {
			// Else if 8 or less validBits, data is unsigned
			// Conversion required dividing by max positive value
			wavFile.floatOffset = 1;
			wavFile.floatScale = 0.5 * ((1 << wavFile.validBits) - 1);
		}

		// Finally, set the IO State
		wavFile.buffer.reset();
		wavFile.frameCounter = 0;
		wavFile.ioState = IOState.WRITING;

		return wavFile;
	}
	
	
	/**
	 * 
	 * @param fileName
	 * @throws IOException 
	 * @throws WavFileException 
	 */
	public static void rewriteFileHeader(String fileName, WavFile wav) throws IOException, WavFileException {
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		newWavFile(os, wav.getNumChannels(), wav.getNumFrames(), wav.getValidBits(), wav.getSampleRate());
		RandomAccessFile f = new RandomAccessFile(fileName, "rw");
		f.write(os.toByteArray());
		os.close();
		f.close();
	}
	
	/**
	 * 
	 * @param wav
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static int getHeaderSize(WavFile wav) throws IOException, WavFileException {
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		newWavFile(os, wav.getNumChannels(), wav.getNumFrames(), wav.getValidBits(), wav.getSampleRate());
		return os.toByteArray().length;
	}

	
	/**
	 * This method opens a wav file stream for reading. It retrieves wav details from the file header. 
	 * Once this method has been called and a WavFile instance retrieved, the readFrames methods can be used for accessing the samples.
	 * 
	 * @param file
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static WavFile openWavStream(InputStream is) throws IOException, WavFileException {
		WavFile wavFile = new WavFile();
		wavFile.iStream = is;
		// Read the first 12 bytes of the file
		int bytesRead = wavFile.buffer.read(wavFile.iStream, 0, 12);
		assertWav(bytesRead == 12, "Not enough wav file bytes for header");

		// Extract parts from the header
		long riffChunkID = wavFile.buffer.getLE(0, 4);
		long chunkSize = wavFile.buffer.getLE(4, 4);
		long riffTypeID = wavFile.buffer.getLE(8, 4);

		// Check the header bytes contains the correct signature
		assertWav(riffChunkID == RIFF_CHUNK_ID, "Invalid Wav Header data, incorrect riff chunk ID");
		assertWav(riffTypeID == RIFF_TYPE_ID, "Invalid Wav Header data, incorrect riff type ID");
		// TODO assertWav(file.length() == chunkSize + 8, "Header chunk size (" + chunkSize + ") does not match file size (" + file.length() + ")");
		
		boolean foundFormat = false;
		boolean foundData = false;

		// Search for the Format and Data Chunks
		while ( true ) {
			// Read the first 8 bytes of the chunk (ID and chunk size)
			bytesRead = wavFile.buffer.read(wavFile.iStream, 0, 8);
			assertWav(bytesRead > 0, "Reached end of file without finding format chunk");
			assertWav(bytesRead == 8, "Reached end of file without finding format chunk");

			// Extract the chunk ID and Size
			long chunkID = wavFile.buffer.getLE(0, 4);
			chunkSize = wavFile.buffer.getLE(4, 4);

			// Word align the chunk size
			// chunkSize specifies the number of bytes holding data. However,
			// the data should be word aligned (2 bytes) so we need to calculate
			// the actual number of bytes in the chunk
			long numChunkBytes = (chunkSize % 2 == 1) ? chunkSize + 1 : chunkSize;

			if ( chunkID == FMT_CHUNK_ID ) {
				// Flag that the format chunk has been found
				foundFormat = true;

				// Read in the header info
				bytesRead = wavFile.buffer.read(wavFile.iStream, 0, 16);

				// Check this is uncompressed data
				int compressionCode = (int) wavFile.buffer.getLE(0, 2);
				assertWav(compressionCode == 1, "Compression Code " + compressionCode + " not supported");

				// Extract the format information
				wavFile.numChannels = (int) wavFile.buffer.getLE(2, 2);
				wavFile.sampleRate = wavFile.buffer.getLE(4, 4);
				wavFile.blockAlign = (int) wavFile.buffer.getLE(12, 2);
				wavFile.validBits = (int) wavFile.buffer.getLE(14, 2);

				assertWav(wavFile.numChannels != 0, "Number of channels specified in header is equal to zero");
				assertWav(wavFile.blockAlign != 0, "Block Align specified in header is equal to zero");
				assertWav(wavFile.validBits >=  2, "Valid Bits specified in header is less than 2");
				assertWav(wavFile.validBits <= 64, "Valid Bits specified in header is greater than 64, this is greater than a long can hold");

				// Calculate the number of bytes required to hold 1 sample
				wavFile.bytesPerSample = (wavFile.validBits + 7) / 8;
				assertWav(wavFile.bytesPerSample * wavFile.numChannels == wavFile.blockAlign, "Block Align does not agree with bytes required for validBits and number of channels");

				// Account for number of format bytes and then skip over
				// any extra format bytes
				numChunkBytes -= 16;
				if ( numChunkBytes > 0 ) {
					wavFile.iStream.skip(numChunkBytes);
				}
			} else if (chunkID == DATA_CHUNK_ID ) {
				// Check if we've found the format chunk,
				// If not, throw an exception as we need the format information
				// before we can read the data chunk
				assertWav(foundFormat, "Data chunk found before Format chunk");

				// Check that the chunkSize (wav data length) is a multiple of the block align (bytes per frame)
				assertWav(chunkSize % wavFile.blockAlign == 0, "Data Chunk size is not multiple of Block Align");

				// Calculate the number of frames
				wavFile.numFrames = chunkSize / wavFile.blockAlign;

				// Flag that we've found the wave data chunk
				foundData = true;

				break;
			} else {
				// If an unknown chunk ID is found, just skip over the chunk data
				wavFile.iStream.skip(numChunkBytes);
			}
		}

		// Throw an exception if no data chunk has been found
		assertWav(foundData, "Did not find a data chunk");

		// Calculate the scaling factor for converting to a normalised double
		if ( wavFile.validBits > 8 ) {
			// If more than 8 validBits, data is signed
			// Conversion required dividing by magnitude of max negative value
			wavFile.floatOffset = 0;
			wavFile.floatScale = 1 << (wavFile.validBits - 1);
		} else {
			// Else if 8 or less validBits, data is unsigned
			// Conversion required dividing by max positive value
			wavFile.floatOffset = -1;
			wavFile.floatScale = 0.5 * ((1 << wavFile.validBits) - 1);
		}

		wavFile.buffer.reset();
		wavFile.frameCounter = 0;
		wavFile.ioState = IOState.READING;

		return wavFile;
	}
	
	/**
	 * This method opens a wav file ready for reading. It retrieves wav details from the file header. 
	 * Once this method has been called and a WavFile instance retrieved, the readFrames methods can be used for accessing the samples.
	 * 
	 * @param file
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static WavFile openWavFile(File file) throws IOException, WavFileException {
		return openWavStream(new FileInputStream(file));
	}



	/**
	 *  Sample Writing
	 *  
	 * @param val
	 * @throws IOException
	 */
	private void writeSample(long val) throws IOException {
		for (int b = 0; b < bytesPerSample; b++) {
			buffer.write(oStream, val);
			val >>= 8;
		}
	}

	/**
	 * 
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	private long readSample() throws IOException, WavFileException {
		long val = 0;

		for (int b = 0; b < bytesPerSample; b++) {
			int v = buffer.read(iStream);
			if ( b < bytesPerSample - 1 || bytesPerSample == 1 ) {
				v &= 0xFF;
			}
			val += v << (b * 8);
		}

		return val;
	}

	/**
	 * Read numFramesToRead frames from the wav file and place into sampleBuffer starting from index 0. 
	 * The user must make sure sampleBuffer contains enough space for all frames to be read 
	 * (required length is the number of channels multiplied by number of frames to be read). 
	 * Samples from each channel are interlaced into the buffer.
	 *
	 * @param sampleBuffer
	 * @param numFramesToRead
	 * @return the number of frames read. When no frames are left to read this method returns 0
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int readFrames(int[] sampleBuffer, int numFramesToRead) throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	/**
	 * Same as above, but sampleBuffer is filled starting at index offset.
	 * 
	 * @param sampleBuffer
	 * @param offset
	 * @param numFramesToRead
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int readFrames(int[] sampleBuffer, int offset, int numFramesToRead)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for (int f = 0; f < numFramesToRead; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++) {
				sampleBuffer[offset] = (int) readSample();
				offset++;
			}

			frameCounter++;
		}

		return numFramesToRead;
	}

	
	/**
	 * Read numFramesToRead frames from the wav file and place into sampleBuffer starting from indices 0,0. 
	 * The sampleBuffer indices are 'Channel Number', 'Frame Number'. For example, sampleBuffer[2][23] is the channel 3 sample from frame 24.
	 * The user must make sure sampleBuffer is the correct dimensions for all channels and frames to be read.
	 * 
	 * @param sampleBuffer
	 * @param numFramesToRead
	 * @return the number of frames read. When no frames are left to read this method returns 0.
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int readFrames(int[][] sampleBuffer, int numFramesToRead) throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	/**
	 * Same as above, but sampleBuffer is filled starting at frame index offset
	 * 
	 * @param sampleBuffer
	 * @param offset
	 * @param numFramesToRead
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int readFrames(int[][] sampleBuffer, int offset, int numFramesToRead)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for (int f = 0; f < numFramesToRead; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++)
				sampleBuffer[c][offset] = (int) readSample();

			offset++;
			frameCounter++;
		}

		return numFramesToRead;
	}

	/**
	 * Write numFramesToWrite frames from sampleBuffer into the wav file starting from index 0. 
	 * The user must make sure sampleBuffer is large enough for all frames to be retrieved for writing 
	 * (required length is the number of channels multiplied by number of frames to be written). 
	 * Samples from each channel are retrieved in an interlaced order.
	 * 
	 * @param sampleBuffer
	 * @param numFramesToWrite
	 * @return the number of frames written. The maximum number of frames that can be written is specified when the WavFile is created using newWavFile. When all frames have been written this method returns 0
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(int[] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}

	/**
	 * Same as above, but sampleBuffer is read starting at index offset.
	 * 
	 * @param sampleBuffer
	 * @param offset
	 * @param numFramesToWrite
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(int[] sampleBuffer, int offset, int numFramesToWrite)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.WRITING, "Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++) {
				writeSample(sampleBuffer[offset]);
				offset++;
			}

			frameCounter++;
		}

		return numFramesToWrite;
	}

	/**
	 * Write numFramesToRead frames from sampleBuffer into the wav file starting from indices 0,0. 
	 * The sampleBuffer indices are 'Channel Number', 'Frame Number'. For example, sampleBuffer[2][23] is the channel 3 sample from frame 24. 
	 * The user must make sure sampleBuffer is the correct dimensions for all channels and frames to be retrieved for writing.
	 * 
	 * @param sampleBuffer
	 * @param numFramesToWrite
	 * @return the number of frames written. The maximum number of frames that can be written is specified when the WavFile is created using newWavFile. When all frames have been written this method returns 0
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(int[][] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}

	/**
	 * Same as above, but sampleBuffer is read starting at frame index offset.
	 * 
	 * @param sampleBuffer
	 * @param offset
	 * @param numFramesToWrite
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(int[][] sampleBuffer, int offset,
			int numFramesToWrite) throws IOException, WavFileException {
		assertIO(ioState == IOState.WRITING, "Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++)
				writeSample(sampleBuffer[c][offset]);

			offset++;
			frameCounter++;
		}

		return numFramesToWrite;
	}

	// Long
	// ----
	public int readFrames(long[] sampleBuffer, int numFramesToRead)
			throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	public int readFrames(long[] sampleBuffer, int offset, int numFramesToRead)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for (int f = 0; f < numFramesToRead; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++) {
				sampleBuffer[offset] = readSample();
				offset++;
			}

			frameCounter++;
		}

		return numFramesToRead;
	}

	public int readFrames(long[][] sampleBuffer, int numFramesToRead)
			throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	public int readFrames(long[][] sampleBuffer, int offset, int numFramesToRead)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for (int f = 0; f < numFramesToRead; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++)
				sampleBuffer[c][offset] = readSample();

			offset++;
			frameCounter++;
		}

		return numFramesToRead;
	}

	public int writeFrames(long[] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}

	public int writeFrames(long[] sampleBuffer, int offset, int numFramesToWrite)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.WRITING, "Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++) {
				writeSample(sampleBuffer[offset]);
				offset++;
			}

			frameCounter++;
		}

		return numFramesToWrite;
	}

	/**
	 * 
	 * @param sampleBuffer
	 * @param numFramesToWrite
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(long[][] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}

	/**
	 * 
	 * @param sampleBuffer
	 * @param offset
	 * @param numFramesToWrite
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public int writeFrames(long[][] sampleBuffer, int offset,
			int numFramesToWrite) throws IOException, WavFileException {
		if (ioState != IOState.WRITING)
			throw new IOException("Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++)
				writeSample(sampleBuffer[c][offset]);

			offset++;
			frameCounter++;
		}

		return numFramesToWrite;
	}

	// Double
	// ------
	public int readFrames(double[] sampleBuffer, int numFramesToRead)
			throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	public int readFrames(double[] sampleBuffer)
			throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, sampleBuffer.length);
	}
	
	public int readFrames(double[] sampleBuffer, int offset, int numFramesToRead)
			throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for ( int f = 0; f < numFramesToRead; f++ ) {
			if ( frameCounter == numFrames ) {
				return f;
			}

			for (int c = 0; c < numChannels; c++) {
				sampleBuffer[offset] = floatOffset + (double) readSample() / floatScale;
				offset++;
			}

			frameCounter++;
		}

		return numFramesToRead;
	}

	public int readFrames(double[][] sampleBuffer, int numFramesToRead)
			throws IOException, WavFileException {
		return readFrames(sampleBuffer, 0, numFramesToRead);
	}

	public int readFrames(double[][] sampleBuffer, int offset,
			int numFramesToRead) throws IOException, WavFileException {
		assertIO(ioState == IOState.READING, "Cannot read from WavFile instance");

		for (int f = 0; f < numFramesToRead; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++)
				sampleBuffer[c][offset] = floatOffset + (double) readSample()
						/ floatScale;

			offset++;
			frameCounter++;
		}

		return numFramesToRead;
	}

	public int writeFrames(double[] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}
	
	public int writeFrames(double[] sampleBuffer)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, sampleBuffer.length);
	}

	public int writeFrames(double[] sampleBuffer, int offset,
			int numFramesToWrite) throws IOException, WavFileException {
		assertIO(ioState == IOState.WRITING, "Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if ( frameCounter == numFrames ) {
				return f;
			}

			for (int c = 0; c < numChannels; c++) {
				writeSample((long) (floatScale * (floatOffset + sampleBuffer[offset])));
				offset++;
			}

			frameCounter++;
		}

		return numFramesToWrite;
	}

	public int writeFrames(double[][] sampleBuffer, int numFramesToWrite)
			throws IOException, WavFileException {
		return writeFrames(sampleBuffer, 0, numFramesToWrite);
	}

	public int writeFrames(double[][] sampleBuffer, int offset,
			int numFramesToWrite) throws IOException, WavFileException {
		assertIO(ioState == IOState.WRITING, "Cannot write to WavFile instance");

		for (int f = 0; f < numFramesToWrite; f++) {
			if (frameCounter == numFrames)
				return f;

			for (int c = 0; c < numChannels; c++) {
				writeSample((long) (floatScale * (floatOffset + sampleBuffer[c][offset])));
			}

			offset++;
			frameCounter++;
		}

		return numFramesToWrite;
	}

	/**
	 * This method opens a wav file ready for reading. It retrieves wav details from the file header. 
	 * Once this method has been called and a WavFile instance retrieved, the readFrames methods can be used for accessing the samples.
	 * 
	 * @throws IOException
	 */
	public void close() throws IOException {
		// Close the input stream and set to null
		if (iStream != null) {
			iStream.close();
			iStream = null;
		}

		if ( oStream != null ) {
			// Write out anything still in the local buffer
			buffer.write(oStream);

			// If an extra byte is required for word alignment, add it to the end
			if ( wordAlignAdjust ) {
				oStream.write(0);
			}

			// Close the stream and set to null
			oStream.close();
			oStream = null;
		}

		// Flag that the stream is closed
		ioState = IOState.CLOSED;
	}

	// TODO remove
	public void display() {
		display(System.out);
	}

	// TODO remove	
	public void display(PrintStream out) {
//		out.printf("File: %s\n", file);
		out.printf("Channels: %d\nFrames: %d\n", numChannels, numFrames);
		out.printf("IO State: %s\n", ioState);
		out.printf("Sample Rate: %d\nBlock Align: %d\n", sampleRate, blockAlign);
		out.printf("Valid Bits: %d\nBytes per sample: %d\n", validBits,	bytesPerSample);
	}
	
	/**
	 * 
	 * @param assertion
	 * @param message
	 * @throws IOException
	 */
	private static void assertIO(boolean assertion, String message) throws IOException {
		if ( !assertion ) {
			throw new IOException(message);
		}
	}
	
	/**
	 * 
	 * @param assertion
	 * @param message
	 * @throws WavFileException
	 */
	private static void assertWav(boolean assertion, String message) throws WavFileException {
		if ( !assertion ) {
			throw new WavFileException(message);
		}
	}
	

	public static void main(String[] args) {
		if (args.length < 1) {
			System.err.println("Must supply filename");
			System.exit(1);
		}

		try {
			for (String filename : args) {
				WavFile readWavFile = openWavFile(new File(filename));
				readWavFile.display();

				long numFrames = readWavFile.getNumFrames();
				int numChannels = readWavFile.getNumChannels();
				int validBits = readWavFile.getValidBits();
				long sampleRate = readWavFile.getSampleRate();

				WavFile writeWavFile = newWavFile(new File("out.wav"), numChannels, numFrames, validBits, sampleRate);

				final int BUF_SIZE = 5001;

				// int[] buffer = new int[BUF_SIZE * numChannels];
				// long[] buffer = new long[BUF_SIZE * numChannels];
				double[] buffer = new double[BUF_SIZE * numChannels];

				int framesRead = 0;
				int framesWritten = 0;

				do {
					framesRead = readWavFile.readFrames(buffer, BUF_SIZE);
					framesWritten = writeWavFile.writeFrames(buffer, BUF_SIZE);
					System.out.printf("read = %d frames, written = %d frames\n", framesRead, framesWritten);
				} while (framesRead != 0);

				readWavFile.close();
				writeWavFile.close();
			}

			WavFile writeWavFile = newWavFile(new File("out2.wav"), 1, 10, 23, 44100);
			double[] buffer = new double[10];
			writeWavFile.writeFrames(buffer, 10);
			writeWavFile.close();
		} catch (Exception e) {
			System.err.println(e);
			e.printStackTrace();
		}
	}
	
	
}
