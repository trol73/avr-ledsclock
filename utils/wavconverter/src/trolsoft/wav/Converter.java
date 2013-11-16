package trolsoft.wav;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;

public class Converter {
	
	public static void removeWavHeader(String src, String out, int len) throws IOException {
		RandomAccessFile inF = new RandomAccessFile(new File(src), "r");
		RandomAccessFile outF = new RandomAccessFile(new File(out), "rw");
		inF.seek(len);
		byte buf[] = new byte[1024*10];
		while ( true ) {
			int readed = inF.read(buf);
			if ( readed < 0 ) {
				break;
			}
			outF.write(buf, 0, readed);
		}
		inF.close();
		outF.close();
	}
	
	private static String setFileExt(String path, String ext) {
		int indx = path.lastIndexOf('.');
		if (indx < 0) {
			return path + '.' + ext;
		}
		return path.substring(0, indx) + '.' + ext;
	}
	
	private static void mp32wav(String mp3Path, String wawPath) {
		
	}
	
	private static void processFile(String srcPath, String outPath, int bits, int sampleRate) throws IOException, WavFileException {
		String sndFileName = setFileExt(outPath, "snd");
		System.out.println("Processing: " + srcPath + " - > " + sndFileName);
		File outFile = new File(sndFileName);
		if (!outFile.getParentFile().exists()) {
			outFile.getParentFile().mkdirs();
		}
		WavFile out = WaveTools.createWav(outPath, 1, bits, sampleRate);
		WaveTools.appendWav(out, srcPath);
		WavFile.rewriteFileHeader(outPath, out);
		out.display();
		out.close();

		int headerLen = WavFile.getHeaderSize(out);
		System.out.println("Header size: " + headerLen);
		
		removeWavHeader(outPath, sndFileName, headerLen);
		
		new File(outPath).delete();	// delete wav
	}
	
	
	private static void processDir(File src, String srcRoot, String outPath, int bits, int sampleRate) throws IOException, WavFileException {
		System.out.println("Processing directory: " + src.getAbsolutePath());
		String rootPath = new File(srcRoot).getAbsolutePath();
		for (File f : src.listFiles()) {
			if (f.isDirectory()) {
				processDir(f, srcRoot, outPath, bits, sampleRate);
			} else {
				String srcPath = f.getAbsolutePath();
				if (!srcPath.toLowerCase().endsWith(".wav")) {
					continue;
				}
				String outName = outPath + srcPath.substring(rootPath.length());
				processFile(srcPath, outName, bits, sampleRate);
			}
		}
	}

	public static void main(String args[]) throws IOException, WavFileException {
		String srcPath = null, outPath = null;
		int bits = 0, sampleRate = 0;
		try {
			srcPath = args[0];
			outPath = args[1];
			bits = Integer.parseInt(args[2]);
			sampleRate = Integer.parseInt(args[3]);
		} catch ( Exception e ) {
			System.out.println("Usage:");
			System.out.println("   wavconverter <src_file.wav> <out_file.wav> <out_bits> <out_sample_rate>");
			System.out.println("   wavconverter <directory with source files> <output directory> <out_bits> <out_sample_rate>");
			System.out.println("Example: wavconverter from.wav to.wav 8 44000");
			return;
		}
		File srcFile = new File(srcPath);
		File outFile = new File(outPath);
		if (!srcFile.exists()) {
			System.out.println("Source doesn't exists: " + srcPath);
			return;
		}
		if (srcFile.isFile()) {
			processFile(srcFile.getAbsolutePath(), outFile.getAbsolutePath(), bits, sampleRate);
		} else if (srcFile.isDirectory()) {
			processDir(srcFile, srcFile.getAbsolutePath(), outFile.getAbsolutePath(), bits, sampleRate);
		}
		
		// 43200 Hz
		
		System.out.println("Done.");
	}


}
