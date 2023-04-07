"""
Utility to convert the GOWIN status code into a human-readable
description.

http://cdn.gowinsemi.com.cn/UG290E.pdf
"""
import sys

if len(sys.argv) != 2:
    print('usage: python3 tools/decode_gowin_status.py 0x0003F020')
    sys.exit(1)
status = int(sys.argv[1], 0)

bitfields = {}
bitfields['0']  = {'high': False, 'desc': 'CRC valid'}
bitfields['1']  = {'high': False, 'desc': 'Command valid'}
bitfields['2']  = {'high': False, 'desc': 'Verify IDCODE valid'}
bitfields['3']  = {'high': False, 'desc': 'Timeout not reached'}
bitfields['12'] = {'high': True,  'desc': 'Gowin embedded flash valid (VLD)'}
bitfields['13'] = {'high': True,  'desc': 'Done Final Status, loading successful'}
bitfields['14'] = {'high': True,  'desc': 'Security Final, security enabled'}
bitfields['15'] = {'high': True,  'desc': 'Ready Status Flag'}
bitfields['16'] = {'high': True,  'desc': 'POR Status ("ERR" when RECONFIG_N is low)'}

for i in bitfields:
    if (status >> int(i) & 1) == bitfields[i]['high']:
        res = 'OK'
    else:
        res = 'ERR'

    desc = bitfields[i]['desc']
    print(f'{res:3}  1<<{i:2}   {desc}')
