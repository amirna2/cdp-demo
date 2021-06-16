package main

import (
	"mqtt-service/database"
	"mqtt-service/mqtt"
)

func main() {
	database.InitDatabase()
	mqtt.StartClient()
}
