import requests
import time
import influx_push
import atexit

chargerIP = "192.168.0.167"
batteryID = int(input("Battery ID: "))
dischargeVoltage = int(input("Ladeschluss Spannung (mV): "))
dischargeCurrent = int(input("Entlade Strom (mA): "))

step = 0
startVoltage = 0
Ri = 0



class Battery:
    voltage = 0.0
    current = 0.0
    power = 0.0
    capacity = 0.0
    energy = 0.0
    def getData(self):
        try:
            charger_data = influx_push.push_data(batteryID)
            self.voltage = float(charger_data["voltage"])
            self.current = abs(float(charger_data["current"]))
            self.power = abs(float(charger_data["watt"]))
            self.capacity = abs(float(charger_data["capacity"]))
            self.energy = abs(float(charger_data["energy"]))
        except Exception as e:
            print (e)

battery = Battery()

while (True):
    if(step == 0):
        requests.get("http://192.168.0.167/reset")
        battery.getData()
        if(battery.voltage >= dischargeVoltage):
            if(battery.current > 20):
                print ("charger was running")
                requests.get("http://192.168.0.167/stop")
                time.sleep(3)
                battery.getData()
            startVoltage = battery.voltage
            requests.get("http://192.168.0.167/discharge?u=" + str(dischargeVoltage) + "&i=" + str(dischargeCurrent))
            step = step + 1
    elif (step == 1):
        battery.getData()
        if(battery.current >= (dischargeCurrent / 1.5)):
            Ri = (startVoltage - battery.voltage) / battery.current
            print ("Ri: " + str(Ri))
            step = step + 1
    elif (step == 2):
        battery.getData()
        if(battery.current <= 50):
            foo = requests.get("http://192.168.0.167/stop")
            print ("Battery: " + str(batteryID))
            print ("Capacity: " + str(battery.capacity) + "mAh")
            print ("Energy: " + str(battery.energy) + "mWh")
            print ("Ri: " + str(Ri))
            step = step + 1
    elif (step == 3):
        break
    time.sleep(1)


