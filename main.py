import asyncio
import sys
from bleak import BleakScanner

def calculate_distance(rssi, a=-59, n=2):
    return 10 ** ((a - rssi) / (10 * n))

async def continuously_scan(interval=0.5):
    print("Starting continuous scanning...")
    
    while True:
        devices = await BleakScanner.discover()
        
        result = "\n--- New Scan ---\n"
        for device in devices:
            distance = calculate_distance(device.rssi)
            result += f"Name: {device.name}, Address: {device.address}, " \
                      f"RSSI: {device.rssi}, Estimated Distance: {distance:.2f} meters\n"
        
        sys.stdout.write("\033c")
        sys.stdout.flush()

        sys.stdout.write(result)
        sys.stdout.flush()
        
        await asyncio.sleep(interval)

asyncio.run(continuously_scan(interval=0.5))
