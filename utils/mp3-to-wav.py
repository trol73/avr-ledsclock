#!/usr/bin/python
# -*- coding: utf-8 -*-


import os, sys, glob

if len(sys.argv) != 3:
	print "usage: mp3-to-wav.py <source mp3/wav dictionary> <output wav directory>"
	sys.exit(1)
	
src = sys.argv[1]
out = sys.argv[2]

if not os.path.exists(out):
	os.system('mkdir ' + out)

# переконвертируем mp3 файлы с сохранением в новый каталог
# имена файлов переводим в нижний регистр. если такой файл уже есть, не трогаем
for infile in glob.glob( os.path.join(src, '*.mp3') ):
	name = os.path.basename(infile)
	word = os.path.splitext(name)[0].lower()
	outName = out + '/' + word + '.wav'
	if os.path.exists(outName):
		continue
	os.system('lame --decode ' + infile + ' ' + out + '/--temp_file_convert')
	os.system('sox ' + out + '/--temp_file_convert' + ' -r 48000 -b 8 -c 1 ' + outName)
	#print 'mpg123 -w ' + infile + ' ' + outName
	#os.system('mpg123 -w ' + outName + ' ' + infile)
	#os.system('lame --decode '  + infile + ' ' + outName)


# wav-файлы просто перекопируем
# имена файлов переводим в нижний регистр. если такой файл уже есть, не трогаем
for infile in glob.glob( os.path.join(src, '*.wav') ):
	name = os.path.basename(infile)
	word = os.path.splitext(name)[0].lower()
	outName = out + '/' + word + '.wav'
	if os.path.exists(outName):
		continue
	os.system('cp ' + infile + ' ' + outName)
	
os.system('rm ' + out + '/--temp_file_convert')

