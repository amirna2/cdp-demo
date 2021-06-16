package database

import (
	"fmt"
	"mqtt-service/sensor"

	"github.com/jinzhu/gorm"
)

var (
	DBConn *gorm.DB
)

func InitDatabase() {
	var err error
	DBConn, err = gorm.Open("sqlite3", "data.db")
	if err != nil {
		panic("failed to connect database")
	}
	fmt.Println("Connection Opened to Database")
	DBConn.AutoMigrate(&sensor.ClusterData{})
	fmt.Println("Database Migrated")
}
