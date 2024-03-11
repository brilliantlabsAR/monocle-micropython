import sys
import asyncio
from brilliant import Monocle

async def main(monocle_fpga_bin):
    async with Monocle() as m:
        await m.send_command('import update')
        await m.send_command('f = update.Fpga')
        await m.send_command('f.erase()')

        with open(monocle_fpga_bin, 'rb') as f:
            while (data := f.read(1024)) != b'':
                await m.send_command(f"f.write({data})")

asyncio.run(main(sys.argv[1]))
