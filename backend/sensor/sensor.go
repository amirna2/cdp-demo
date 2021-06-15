package sensor

import (
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
)

type Data struct {
	gorm.Model
	DateTime  string
	DeviceId  string `json:"DeviceID"`
	MessageID string `json:"MessageID"`
	Payload   string `json:"Payload"`
	Path      string `json:"path"`
	Hops      int    `json:"hops"`
	DuckType  int    `json:"duckType"`
	Topic     int    `json:"topic"`
}
