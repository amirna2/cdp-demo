package mqtt

import (
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"mqtt-service/database"
	"mqtt-service/sensor"
	"os"
	"os/signal"
	"syscall"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var client mqtt.Client

var messagePubHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	sensorData := new(sensor.ClusterData)
	sensorData.TimeStamp = time.Now().Local()
	fmt.Printf("[%s] %s\n", sensorData.TimeStamp, msg.Payload())

	json.Unmarshal(msg.Payload(), &sensorData)
	database.DBConn.Create(&sensorData)

	appData, err := json.Marshal(sensorData)
	if err != nil {
		fmt.Printf("Failed to create app data: %s\n", err.Error())
	} else {
		Publish(appData)
	}

}

var connectHandler mqtt.OnConnectHandler = func(client mqtt.Client) {
	fmt.Println("Connected")
}

var connectionLostHandler mqtt.ConnectionLostHandler = func(client mqtt.Client, err error) {
	fmt.Printf("Connection Lost: %s\n", err.Error())
}

func newTLSConfig() *tls.Config {
	certpool := x509.NewCertPool()
	ca, err := ioutil.ReadFile("./certs/mosquitto.org.crt")
	if err != nil {
		panic(err.Error())
	}

	certpool.AppendCertsFromPEM(ca)
	return &tls.Config{RootCAs: certpool}
}

func StartClient() {
	fmt.Printf("Starting MQTT client\n")
	interrupt := make(chan os.Signal, 1)
	signal.Notify(interrupt, os.Interrupt, syscall.SIGTERM)

	// var broker = "tcp://test.mosquitto.org:1883"
	var broker = "ssl://test.mosquitto.org:8883"

	options := mqtt.NewClientOptions()
	options.AddBroker(broker)
	options.SetClientID("cdp-app-server")
	options.SetDefaultPublishHandler(messagePubHandler)
	options.OnConnect = connectHandler
	options.OnConnectionLost = connectionLostHandler
	options.SetTLSConfig(newTLSConfig())
	options.SetKeepAlive(60 * 2 * time.Second)

	client = mqtt.NewClient(options)
	token := client.Connect()
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	topic := "/test-mosquitto/status/json"
	token = client.Subscribe(topic, 1, nil)
	token.Wait()
	fmt.Printf("Subscribed to topic %s\n", topic)
	<-interrupt

	client.Disconnect(100)
}

func Publish(data []byte) {
	if !client.IsConnected() {
		fmt.Printf("Error. Mqtt client not connected.")
		return
	}
	token := client.Publish("cdp-app-server/sensor", 2, false, data)
	if token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	fmt.Println("Published cdp-app-server/sensor topic")

}
