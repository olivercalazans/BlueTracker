import asyncio, sys
from bleak import BleakScanner


class BlueTracker:

    @staticmethod
    async def _start_scan(interval=0.5) -> None:
        try:   BlueTracker._continuously_scan(interval)
        except KeyboardInterrupt:  print('Scan stoped')
        except Exception as error: print(f'Unknown errer: {error}')


    @staticmethod
    async def _continuously_scan(interval:float|int) -> None:
        print("Starting continuous scanning...")
        while True:
            devices = await BleakScanner.discover()
            devices = [(dev.name, dev.address, dev.rssi) for dev in devices]
            devices = sorted(devices, key=lambda x:x[-1])
            result  = [f'Estimated Distance: {BlueTracker._get_distance(dev[2]):<14} meters, Name: {dev[0]}, Address: {dev[1]}' for dev in devices]
            result  = '\n'.join(result)
            BlueTracker._display_result(result)
            await asyncio.sleep(interval)


    @staticmethod
    def _calculate_distance(rssi, a=-59, n=2) -> int:
        return 10 ** ((a - rssi) / (10 * n))


    @staticmethod
    def _get_distance(rssi:int) -> str:
        rssi = BlueTracker._calculate_distance(rssi)
        if   rssi > 10: return BlueTracker.red(f'{rssi:.2f}')
        elif rssi > 5:  return BlueTracker.yellow(f'{rssi:.2f}')
        else:           return BlueTracker.green(f'{rssi:.2f}')


    @staticmethod
    def _display_result(result:str) -> None:
        sys.stdout.write("\033c")
        sys.stdout.flush()
        sys.stdout.write(result)
        sys.stdout.flush()


    # COLORS ------------------------------------------------------
    @staticmethod
    def green(message:str|int) -> str:
        return f'\033[32m{message}\033[0m'

    @staticmethod
    def red(message:str|int) -> str:
        return f'\033[31m{message}\033[0m'

    @staticmethod
    def yellow(message:str|int) -> str:
        return f'\033[33m{message}\033[0m'



asyncio.run(BlueTracker._start_scan(interval=1))
