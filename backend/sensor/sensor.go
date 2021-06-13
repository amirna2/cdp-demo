package sensor

import (
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
)

type Data struct {
	gorm.Model
	DateTime  string `json:"timestamp"`
	DeviceId  string `json:"DeviceID"`
	MessageID string `json:"MessageID"`
	Path      string `json:"path"`
	Hops      int    `json:"hops"`
	DuckType  int    `json:"duckType"`
	Topic     int    `json:"topic"`
}
