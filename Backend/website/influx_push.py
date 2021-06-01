from influxdb import InfluxDBClient
import requests
import time

client = InfluxDBClient(host='xxxxx', port=8086, username='xxxxxx', password='xxxxx')

client.create_database('ESP-Charger')
client.switch_database('ESP-Charger')

def push_data(_batteryID):
    try:
        charger_data = requests.get('http://192.168.0.167/getdata').json()
        #print(charger_data)
        json_body = [
            {
                "measurement": "ESP-Charger",
                "tags": {
                    "battery": _batteryID,
                },
                "fields": {
                    "voltage": charger_data["voltage"],
                    "current": abs(charger_data["current"]),
                    "power": abs(charger_data["watt"]),
                    "capacity": abs(charger_data["capacity"]),
                    "energy": abs(charger_data["energy"]),
                    "chargePower": charger_data["chargePower"],
                    "dischargePower": charger_data["dischargePower"]
                }
            }
        ]
        client.write_points(json_body)
        print(json_body)
        return charger_data
    except:
        print ("error")
