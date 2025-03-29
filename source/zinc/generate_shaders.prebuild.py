#!/usr/bin/env python3
import sys
import os
from pathlib import Path

def makestr(name):
	p = Path(name)
	varname = p.name.replace('.', '_')
	
	x = p.read_text().replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t").replace("\"", "\\\"")
	return f'const char *shader_{varname} = "{x}";\n'

def main():
	f = open("shaders.h", "w")
	
	for item in os.listdir('shaders'):
		f.write(makestr(f'shaders/{item}'))
	
	f.close()

if __name__ == '__main__':
	main()
