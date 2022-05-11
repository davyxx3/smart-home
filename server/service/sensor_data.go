package service

import (
	"smart-home/model"
	"smart-home/sql"
	"smart-home/utils/errmsg"
	"time"
)

func StoreSensorData(sensorData model.SensorData) int {
	result := sql.Db.Create(&sensorData)
	if result.Error != nil {
		return errmsg.ERROR
	}
	return errmsg.SUCCESS
}

func GetSensorData(devName string, begin time.Time, end time.Time, pageNum int, pageSize int) ([]model.SensorData, int, int) {
	var dataGroup []model.SensorData
	result := sql.Db.Limit(pageSize).Offset((pageNum-1)*pageSize).Where("dev_name = ? AND time between ? and ?", devName, begin, end).Find(&dataGroup)
	if result.Error != nil {
		return nil, -1, errmsg.ERROR
	}
	var total int64
	sql.Db.Model(&model.SensorData{}).Where("dev_name = ? AND time between ? and ?", devName, begin, end).Count(&total)

	// 格式化时间
	for index := range dataGroup {
		t, _ := time.Parse(time.RFC3339, dataGroup[index].Time)
		dataGroup[index].Time = t.Format("2006-01-02 15:04:05")
	}

	return dataGroup, int(total), errmsg.SUCCESS
}

func GetSensorDataDescendWithLimit(devName string, limit int) ([]model.SensorData, int) {
	var dataGroup []model.SensorData
	result := sql.Db.Where("dev_name = ?", devName).Order("id desc").Limit(limit).Find(&dataGroup)
	if result.Error != nil {
		return nil, errmsg.ERROR
	}

	// 格式化时间
	for index, _ := range dataGroup {
		t, _ := time.Parse(time.RFC3339, dataGroup[index].Time)
		dataGroup[index].Time = t.Format("15:04")
	}

	return dataGroup, errmsg.SUCCESS
}

func GetAllSensorData() ([]model.SensorData, int, int) {
	var dataGroup []model.SensorData
	result := sql.Db.Find(&dataGroup)
	if result.Error != nil {
		return nil, -1, errmsg.ERROR
	}
	return dataGroup, int(result.RowsAffected), errmsg.SUCCESS
}
