package main

import (
	"backend/database"
	"backend/sensor"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"os/signal"
	"syscall"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/jinzhu/gorm"
)

var messagePubHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	sensorData := new(sensor.Data)
	sensorData.DateTime = time.Now().Format("2006.01.02 15:04:05")
	fmt.Printf("[%s] %s\n", sensorData.DateTime, msg.Payload())

	json.Unmarshal(msg.Payload(), &sensorData)

	db := database.DBConn
	db.Create(&sensorData)
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

func initDatabase() {
	var err error
	database.DBConn, err = gorm.Open("sqlite3", "data.db")
	if err != nil {
		panic("failed to connect database")
	}
	fmt.Println("Connection Opened to Database")
	database.DBConn.AutoMigrate(&sensor.Data{})
	fmt.Println("Database Migrated")
}

func startMqttClient() {
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

	client := mqtt.NewClient(options)
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

func main() {
	initDatabase()
	startMqttClient()
}
