package model

import (
	"gorm.io/gorm"
)

type DeviceData struct {
	gorm.Model
	DevId   int     `gorm:"type: int;not null" json:"dev_id"`
	DevName string  `gorm:"type: varchar(20)" json:"dev_name"`
	DevVol  float64 `gorm:"type: double" json:"dev_vol"`
	DevBat  float64 `gorm:"type: double" json:"dev_bat"`
	DevAng  float64 `gorm:"type: double" json:"dev_ang"`
	DevSig  float64 `gorm:"type: double" json:"dev_sig"`
}
