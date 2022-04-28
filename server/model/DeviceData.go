package model

import (
	"gorm.io/gorm"
	"smart-home/utils/errmsg"
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

func StoreDeviceData(deviceData DeviceData) int {
	result := db.Create(&deviceData)
	if result.RowsAffected < 1 {
		return errmsg.ERROR
	}
	return errmsg.SUCCESS
}

func GetDeviceData() (DeviceData, int) {
	var deviceData DeviceData
	result := db.Last(&deviceData)
	if result.RowsAffected < 1 {
		return DeviceData{}, errmsg.ERROR
	}
	return deviceData, errmsg.SUCCESS
}
