package sensor

import (
	"time"

	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
)

type ClusterData struct {
	gorm.Model
	TimeStamp time.Time `json:"timestamp"`
	DeviceId  string    `json:"DeviceID"`
	Topic     string    `json:"topic"`
	MessageID string    `json:"MessageID"`
	Payload   string    `json:"Payload"`
	Path      string    `json:"path"`
	Hops      int       `json:"hops"`
	DuckType  int       `json:"duckType"`
}
