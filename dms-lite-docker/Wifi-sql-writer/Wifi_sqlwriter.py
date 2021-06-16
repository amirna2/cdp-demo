#This script takes the incoming messages and writes them to the DB  & MQTT
from time import gmtime, strftime
import paho.mqtt.client as mqtt
import sqlite3
from sqlite3 import Error
import json
import logging

status_topic = "/test-mosquitto/status/json"
dbFile = "/db/data.db"
logging.warning("Write to DB is Running")

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    print(client)

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(status_topic)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    theTime = strftime("%Y-%m-%d %H:%M:%S", gmtime())

    result = (theTime + "\t" + str(msg.payload))
    print(msg.topic + ":\t" + result)
    # if (msg.topic == status_topic):
    p = json.loads(msg.payload)
    print (json.dumps(p))
    print("New message recieved")
    logging.warning("New message recieved")
    print(["topic"])
    writeToDb(theTime, p["DeviceID"], p["topic"], p["MessageID"], p["Payload"], p["path"],p["hops"],p["duckType"])
    return

def writeToDb(theTime, duckId, topic, messageId, payload, path, hops, duckType):
    conn = sqlite3.connect(dbFile)
    c = conn.cursor()
    print ("Writing to db...")
    try:
        c.execute("INSERT INTO ClusterData VALUES (?,?,?,?,?,?,?,?)", (theTime, duckId, topic, messageId, payload, path, hops, duckType))
        conn.commit()
        conn.close()
    except Error as e:
        print("Not Correct Packet")
        print(e)


logging.warning("Creating Client")
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

logging.warning("connecting....")

client.connect("test.mosquitto.org", 1883, 60)
      
try:
    db = sqlite3.connect(dbFile)
    db.cursor().execute("CREATE TABLE IF NOT EXISTS clusterData (timestamp datetime, duck_id TEXT, topic TEXT, message_id TEXT, payload TEXT, path TEXT, hops INT, duck_type INT)")
    db.commit()
    db.close()
except  Error as e:
    print(e)


# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
