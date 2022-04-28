package model

import (
	"gorm.io/gorm"
)

type Data struct {
	gorm.Model
	DevId   int        `gorm:"type: int;not null" json:"dev_id"`
	DevName string     `gorm:"type: varchar(20)" json:"dev_name"`
	DevData DeviceData `gorm:"embedded" json:"dev_data"`
	SenData SensorData `gorm:"embedded" json:"sen_data"`
}
