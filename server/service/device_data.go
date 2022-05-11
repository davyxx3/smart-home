package service

import (
	"smart-home/model"
	"smart-home/sql"
	"smart-home/utils/errmsg"
)

func StoreDeviceData(deviceData model.DeviceData) int {
	result := sql.Db.Create(&deviceData)
	if result.Error != nil {
		return errmsg.ERROR
	}
	return errmsg.SUCCESS
}

func GetDeviceData(devName string) (model.DeviceData, int) {
	var deviceData model.DeviceData
	result := sql.Db.Where("dev_name = ?", devName).Find(&deviceData)
	if result.Error != nil {
		return model.DeviceData{}, errmsg.ERROR
	}
	return deviceData, errmsg.SUCCESS
}

func GetDeviceList() ([]model.DeviceData, int) {
	var dataGroup []model.DeviceData
	result := sql.Db.Distinct("dev_name").Find(&dataGroup)
	if result.Error != nil {
		return nil, errmsg.ERROR
	}
	return dataGroup, errmsg.SUCCESS
}
