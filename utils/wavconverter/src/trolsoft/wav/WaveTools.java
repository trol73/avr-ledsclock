package trolsoft.wav;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class WaveTools {

	
	/**
	 * 
	 * @param src
	 * @param sampleRateSrc
	 * @param sampleRateOut
	 * @return
	 */
	public static double[] resampleData(double[] src, long sampleRateSrc, long sampleRateOut) {
		if (sampleRateOut == sampleRateSrc) {
			return src;
		}

		final int n = src.length;
		final int samples = (int) (n * sampleRateOut / sampleRateSrc);
		final double s = 1.0*sampleRateSrc / sampleRateOut;
		double x = 0;
		int i = 0;
		double[] res = new double[samples];

		if ( sampleRateSrc > sampleRateOut ) {
			for ( int j = 0; j < samples; j++ ) {
				double p0 = s * j;
				double p1 = p0 + s;
				if ( p0 >= i ) {
					x = src[i++];
				}
				double z = (i - p0) * x;
				while ( p1 >= i && i < n ) {
					x = src[i++];
					z += x;
				}
				z -= (i - p1) * x;
				res[j] = z;
			}

		} else {
			double p1 = 0;
			double y = 0;
			for ( int j = 0; j < samples; j++ ) {
				p1 += s;
				if ( p1 > i ) {
					y = x;
					x = i == n ? 0 : src[i++];
				}
				res[j] = x + (y - x) * (i - p1);
			}
		}
		return res;
	}
	
	
	/**
	 * 
	 * @param ch1
	 * @param ch2
	 * @return
	 */
	private static double[] makeStereo(double[] ch1, double[] ch2) {
		if ( ch1.length != ch2.length ) {
			throw new RuntimeException("number of smaples must be equals");
		}
		double[] result = new double[ch1.length*2];
		for ( int i = 0; i < ch1.length; i++ ) {
			result[i*2] = ch1[i];
			result[i*2+1] = ch2[i];
		}
		return result;
	}
	
	/**
	 * 
	 * @param stereo
	 * @return
	 */
/*	
	private static double[] makeMono(double[] stereo) {
//		if ( stereo.length % 2 != 0 ) {
//			throw new RuntimeException("invalid stareo length");
//		}
		double[] result = new double[stereo.length/2];
		for ( int i = 0; i < result.length; i++ ) {
			result[i] = (stereo[2*i] + stereo[2*i+1])/2.0;
		}
		return result;
	}
*/
	
	/**
	 * 
	 * @param stereo
	 * @return
	 */
	private static double[] makeMono(double[] ch1, double[] ch2) {
		if ( ch1.length != ch2.length ) {
			throw new RuntimeException("number of smaples must be equals");
		}
		double[] result = new double[ch1.length];
		for ( int i = 0; i < result.length; i++ ) {
			result[i] = (ch1[i] + ch2[i])/2.0;
		}
		return result;
	}
	
	

	/**
	 * 
	 * @param stereo
	 * @return
	 */
	private static double[] getChannel1(double stereo[]) {
		double[] result = new double[stereo.length/2];
		for ( int i = 0; i < result.length; i++ ) {
			result[i] = stereo[2*i];
		}
		return result;
	}

	
	/**
	 * 
	 * @param stereo
	 * @return
	 */
	private static double[] getChannel2(double stereo[]) {
		double[] result = new double[stereo.length/2];
		for ( int i = 0; i < result.length; i++ ) {
			result[i] = stereo[2*i + 1];
		}
		return result;
	}
	
	/**
	 * 
	 * @param fileName
	 * @param numChannels
	 * @param validBits
	 * @param sampleRate
	 * @return
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static WavFile createWav(String fileName, int numChannels, int validBits, int sampleRate) throws IOException, WavFileException {
		WavFile wav = WavFile.newWavFile(new File(fileName), numChannels, 0, validBits, sampleRate);
		return wav;
	}
	
	
	/**
	 * 
	 * @param src
	 * @param fileName
	 * @throws WavFileException 
	 * @throws IOException 
	 */
	public static void appendWav(WavFile src, InputStream stream) throws IOException, WavFileException {
		WavFile readWavFile = WavFile.openWavStream(stream);
		//readWavFile.display();

		long numFrames = readWavFile.getNumFrames();
		int addedChannels = readWavFile.getNumChannels();
		int srcChannels = src.getNumChannels();

		double sourceData[] = new double[(int)(numFrames*addedChannels)];
		int framesRead = readWavFile.readFrames(sourceData);
		if ( framesRead != numFrames ) {
			throw new RuntimeException("frames reading error");
		}
		
		
		double convertedData[] = null;
		double convertedDataCh1[] = null;
		double convertedDataCh2[] = null;
		if ( addedChannels == 1 ) {
			convertedData = WaveTools.resampleData(sourceData, readWavFile.getSampleRate(), src.getSampleRate());
		} else if ( addedChannels == 2 ) {
			double[] ch1 = getChannel1(sourceData);
			double[] ch2 = getChannel2(sourceData);
			convertedDataCh1 = WaveTools.resampleData(ch1, readWavFile.getSampleRate(), src.getSampleRate());
			convertedDataCh2 = WaveTools.resampleData(ch2, readWavFile.getSampleRate(), src.getSampleRate());
		} else {
			throw new RuntimeException("unsupported number of channels");	
		}
		
		
		if ( addedChannels == 1 && srcChannels == 1 ) {
			// convertedData = convertedData;
		} else if ( addedChannels == 2 && srcChannels == 2 ) {
			convertedData = makeStereo(convertedDataCh1, convertedDataCh2);
		} else if ( addedChannels == 2 && srcChannels == 1 ) {
			convertedData = makeMono(convertedDataCh1, convertedDataCh2);
		} else if ( addedChannels == 1 && srcChannels == 2 ) {
			convertedData = makeStereo(convertedData, convertedData);
		} else {
			throw new RuntimeException("unsupported file: channels = " + addedChannels);
		}
		

		src.setNumFrames(src.getNumFrames() + convertedData.length);
		
		int framesWritten = src.writeFrames(convertedData);
		if ( framesWritten != convertedData.length ) {
			throw new RuntimeException("frames writing error: " + framesWritten + " from " + convertedData.length);
		}
	}
	
	/**
	 * 
	 * @param src
	 * @param fileName
	 * @throws IOException
	 * @throws WavFileException
	 */
	public static void appendWav(WavFile src, String fileName) throws IOException, WavFileException {
		appendWav(src, new FileInputStream(new File(fileName)));
	}
	
	/**
	 * 
	 * @param src
	 * @param time
	 * @throws WavFileException 
	 * @throws IOException 
	 */
	public static void appendPause(WavFile src, double time) throws IOException, WavFileException {
		int framesNum = (int)(time*src.getSampleRate());
		double[] frames = new double[framesNum];
		for ( int i = 0; i < framesNum; i++ ) {
			frames[i] = 0;
		}
		src.setNumFrames(src.getNumFrames() + framesNum);
		
		int framesWritten = src.writeFrames(frames);
		if ( framesWritten != framesNum ) {
			throw new RuntimeException("frames writing error: " + framesWritten + " from " + framesNum);
		}
	}
	
	
	
	
	
	
	public static void main(String[] args) throws IOException, WavFileException {
		String outName = "/home/trol/OUT.wav";
		String srcName1 = "/home/trol/projects/python/english/words-all/zealous.wav";		
		String srcName2= "/home/trol/projects/python/english/words-all/abandon.wav";
		
				
		WavFile wav = createWav(outName, 1, 8, 44000);
		appendWav(wav, srcName1);
		appendPause(wav, 1);
		appendWav(wav, srcName2);
		WavFile.rewriteFileHeader(outName, wav);
		wav.display();
		wav.close();

		System.out.println("------------------");
		WavFile readWavFile = WavFile.openWavFile(new File(outName));
		readWavFile.display();
		readWavFile.close();

/*  
		WavFile readWavFile = WavFile.openWavFile(new File(srcName));
		readWavFile.display();

		long numFrames = readWavFile.getNumFrames();
		int numChannels = readWavFile.getNumChannels();
		int validBits = readWavFile.getValidBits();
		long sampleRate = readWavFile.getSampleRate()*0 + 44000;
		
		double data[] = new double[(int)numFrames];
		int framesRead = readWavFile.readFrames(data, data.length);
		if ( framesRead != numFrames ) {
			throw new RuntimeException("frames reading error");
		}
		double converted[] = WaveTools.resampleData(data, readWavFile.getSampleRate(), sampleRate);
		numFrames = converted.length;
		WavFile writeWavFile = WavFile.newWavFile(new File(outName), numChannels, numFrames, validBits, sampleRate);
		
		
		int framesWritten = writeWavFile.writeFrames(converted, converted.length);
		if ( framesWritten != converted.length ) {
			throw new RuntimeException("frames writing error");
		}
		
		readWavFile.close();
		writeWavFile.close();
		
		System.out.println("------------------");
		readWavFile = WavFile.openWavFile(new File(outName));
		readWavFile.display();
		readWavFile.close();
*/		
	}

}
