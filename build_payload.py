import sys, os

source = 'raw_testvideo.h264'
output = 'VIDEO.264'
with open(source, 'rb') as fsrc, open(output, 'wb') as fdst:
    data = fsrc.read()
    fdst.write(data)

print(f'Wrote {len(data)} bytes to {output}')
