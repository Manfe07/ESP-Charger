import json
import requests

def get_values():
    json_data = requests.get('http://192.168.0.167/getdata')

    #json_data={
    #  "voltage": 3853.08,
    #  "current": -214.02,
    #  "capacity": -0.46,
    #  "chargePower": 0.00,
    #  "dischargePower": 39.06
    #}
    return json_data.json()
