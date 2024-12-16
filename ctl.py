import paho.mqtt.client as mqtt

# MQTT 配置
broker = "xiaozhuiot.cn"                # MQTT Broker 地址
port = 1883                             # MQTT Broker 端口
username = "xiaozhu"                    # 用户名
password = "ieTOIugSDfieWw23gfiwefg"    # 密码

# MQTT 消息内容
# topic = "/XZBGS3DOWN/FFFF"              # 广播主题
topic = "/XZRCU/DOWN/983DAEE4F5C8"
topic = "/XZRCU/DOWN/983DAEE4FDF0"


payload = '{"type":"ctl","devicetype":"light","deviceid":"2","operation":"打开","param":"0"}'
payload = '{"type":"ctl","devicetype":"curtain","deviceid":"8","operation":"打开","param":"0"}'
payload = '{"type":"ctl","devicetype":"mode","deviceid":"0","operation":"电视模式","param":"0"}'
payload = '{"type":"urge","devicetype":"mode","deviceid":"0","operation":"电视模式","param":"0"}'

# 创建 MQTT 客户端
client = mqtt.Client()

# 设置用户名和密码
client.username_pw_set(username, password)

# 连接到 MQTT Broker
client.connect(broker, port)

# 发布消息
client.publish(topic, payload)

# 断开连接
client.disconnect()

print(f"Message published to topic '{topic}' with payload: '{payload}'")