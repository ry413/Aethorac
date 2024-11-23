import socket
import json

esp32_ip = "192.168.2.21"
esp32_port = 8080

with open("/Users/ry79/Downloads/rcu_config.json", "r") as file:
    json_data = json.load(file)
json_string = json.dumps(json_data, ensure_ascii=False)
print("JSON data to send:", json_string)


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((esp32_ip, esp32_port))
    s.sendall(json_string.encode('utf-8'))
    print("JSON data sent to ESP32")