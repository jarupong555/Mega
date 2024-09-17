import threading
from datetime import datetime
import psutil
import socket
import serial
import queue
import json
import sys
import os
from time import sleep
VERSION = "v2.0.2-alpha"
CONFIG_PATH = "/opt/ns-manager/config.json"


# create a queue to store data received over UDP
udp_data_queue = queue.Queue()


def loadConfig():
    f = open(CONFIG_PATH)
    data = json.load(f)
    print("# configuration data: " + str(data))
    f.close()
    return data


config = loadConfig()
ROOMNAME = config["roomname"]
SERVER = config['server']
SERVER_PORT = 24949  # config['port']
RECEIVE_PORT = 5555
INTERVAL = config['interval']

# UDP receive thread
def udp_receive_thread(udp_socket):
    while True:
        data, addr = udp_socket.recvfrom(1024)  # receive 1024 bytes of data
        print(f"{data}, addr: {addr}")
        # process data here
        udp_data_queue.put(data)  # add data to the queue

# serial send thread
def serial_send_thread(serial_port):
    while True:
        data = udp_data_queue.get()  # get data from the queue
        # send data through the serial port
        # serial_port.write(b'some data\n')
        serial_port.write(data[:-2])
        serial_port.write(f"\n".encode("utf-8"))

# send packet to server
def sendpacket(sock, message):
    try:
        # sock.sendto(message.encode('utf-8'), ("192.168.1.103", 65535))
        sock.sendto(message.encode('utf-8'), (SERVER, SERVER_PORT))
    except:
        print("unsend")
    finally:
        print("send UDP" + message)

# serial recieve thread
def serial_recieve_thread(ser, sock):
    while True:
        received_data = ser.read()  # read serial port
        sleep(0.03)
        data_left = ser.inWaiting()  # check for remaining byte
        received_data += ser.read(data_left)
        # print (received_data)                   #print received data
        try:
            temp = ((received_data).decode("utf-8")).split("\r\n")
            # temp = ((received_data).decode("utf-8"))[:-2]
            # print(temp)
            for item in temp:
                # print(f"from mega: {item}, data type: {type(item)}")
                if item == "callGreen":
                    print(" >> call Green is pressed")
                    sendpacket(sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" +
                               f"{ROOMNAME}" + "-001\"")  # Green
                elif item == "callRed":
                    print(" >> call Red is pressed")
                    sendpacket(
                        sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-004\"")  # Red
                elif item == "pendant":
                    print(" >> pendant is pressed")
                    sendpacket(
                        sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-007\"")
                elif item == "pullcord":
                    print(" >> pullcord is pressed")
                    sendpacket(
                        sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-008\"")
                elif item == "exCall":
                    print(" >> Ex Call is pressed")
                    sendpacket(
                        sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-011\"")
                elif item == "exCall1":
                    print(" >> Ex Call1 is pressed")
                    sendpacket(
                        sock, f":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-012\"")
                elif item == "canGreen":
                    print(" >> cancel Green call is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-001\"")
                elif item == "canRed":
                    print(" >> cancel Red call is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-004\"")
                elif item == "canPendant":
                    print(" >> cancel pendant is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-007\"")
                elif item == "canPullcord":
                    print(" >> cancel Pullcprd is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-008\"")
                elif item == "canExcall":
                    print(" >> cancel Ex call is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-011\"")
                elif item == "canExcall1":
                    print(" >> cancel Ex call1 is pressed")
                    sendpacket(
                        sock, f":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"" + f"{ROOMNAME}" + "-012\"")
        except:
            print("!!can't decode")
            pass

def get_ip_address(ifname):
    temp = os.popen('ip addr').read()
    strEth0 = temp[temp.index(ifname):]
    strInet = strEth0[strEth0.index("inet"):]
    strIP = strInet[strInet.index(' ') + 1:strInet.index('/')]
    return strIP

def get_temperature():
    with open("/sys/class/thermal/thermal_zone0/temp") as f:
        temp_raw = f.read()
    temp_c = int(temp_raw) / 1000.0
    return temp_c

def get_system_stat():
    system_stat = dict()
    try:
        system_stat["cpu_percent"] = psutil.cpu_percent()
        system_stat["virtual_mem_percent"] = psutil.virtual_memory().percent
        system_stat["cpu_temp"] = float(f"{get_temperature():.2f}")
    except:
        print("# psutil failed!")
    # print(system_stat)
    return system_stat

def udp_heartbeat_thread(delay):
    global ROOMNAME, SERVER, PORT
    sleep(30)
    local_ip = get_ip_address('eth0')
    sock = socket.socket(socket.AF_INET,  # Internet
                         socket.SOCK_DGRAM)  # UDP
    while True:
        try:
            msg = get_system_stat()
        except:
            msg = dict()
        msg["roomname"] = ROOMNAME
        msg["ipaddress"] = local_ip
        msg["message_type"] = "heartbeat"

        now = datetime.now()
        msg["time"] = now.strftime("%Y-%m-%d %H:%M:%S")
        msg = json.dumps(msg)
        # print(msg)
        sock.sendto(msg.encode('utf-8'), (SERVER, SERVER_PORT))
        # client.publish(MQTT_TOPICS["heartbeat"], msg)
        sleep(delay)


if __name__ == "__main__":
    temperature = get_temperature()
    print(f"On start Temperature: {temperature:.2f}°C")

    # create a UDP socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind(('0.0.0.0', RECEIVE_PORT))  # bind to port #####

    # create a serial port
    serial_port = serial.Serial('/dev/ttyS3', 115200)

    print(f"Start!")
    serial_port.write(f"Ready\n".encode("utf-8"))

    # start the UDP receive thread
    udp_receive_thread = threading.Thread(
        target=udp_receive_thread, args=(udp_socket,))
    udp_receive_thread.start()

    # start the serial send thread
    serial_send_thread = threading.Thread(
        target=serial_send_thread, args=(serial_port,))
    serial_send_thread.start()

    # start the serial recieve thread
    serial_recieve_thread = threading.Thread(
        target=serial_recieve_thread, args=(serial_port, udp_socket,))
    serial_recieve_thread.start()

    # start the heartbeat thread
    udp_heartbeat_thread = threading.Thread(
        target=udp_heartbeat_thread, args=(INTERVAL,))
    udp_heartbeat_thread.start()
