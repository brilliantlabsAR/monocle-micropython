import asyncio
from brilliant import Monocle

async def main():
    async with Monocle() as m:
        await m.send_command(f"import bluetooth")
        await m.send_command(f"bluetooth.send('hello world!')")
        result = (await m.get_all_data()).decode('ascii')
        print(result)

asyncio.run(main())
