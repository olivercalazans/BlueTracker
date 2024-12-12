# MIT License
# Copyright (c) 2024 Oliver Ribeiro Calazans Jeronimo
# Repository: https://github.com/olivercalazans/BlueTracker
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software...


import asyncio, sys
from functools import lru_cache
from bleak import BleakScanner


class BlueTracker:

    @staticmethod
    async def _start_scan(interval=0.5) -> None:
        try:   await BlueTracker._continuously_scan(interval)
        except KeyboardInterrupt:  print('Scan stoped')
        except Exception as error: print(f'Unknown errer: {error}')


    async def _continuously_scan(interval: float | int) -> None:
        print("Starting continuous scanning...")
        while True:
            devices = await BleakScanner.discover()
            asyncio.create_task(BlueTracker._process_devices(devices))
            await asyncio.sleep(interval)


    @staticmethod
    async def _process_devices(devices) -> None:
        devices = [(dev.name, dev.address, dev.rssi, BlueTracker._calculate_distance(dev.rssi)) for dev in devices]
        devices = sorted(devices, key=lambda x: x[-1])
        result  = [f'Estimated Distance: {BlueTracker._color(dev[3]):<14} meters, Name: {dev[0]}, Address: {dev[1]}' for dev in devices]
        result  = '\n'.join(result)
        BlueTracker._display_result(result)


    @staticmethod
    @lru_cache(maxsize=50)
    def _calculate_distance(rssi, a=-59, n=2) -> int:
        return 10 ** ((a - rssi) / (10 * n))


    @staticmethod
    def _color(distance:int) -> str:
        if   distance > 10: return BlueTracker.red(f'{distance:.2f}')
        elif distance > 5:  return BlueTracker.yellow(f'{distance:.2f}')
        else:               return BlueTracker.green(f'{distance:.2f}')


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



asyncio.run(BlueTracker._start_scan(interval=0.5))
