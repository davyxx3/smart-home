package model

import (
	"fmt"
	"gorm.io/gorm"
	"smart-home/utils/errmsg"
	"time"
)

type TimeFormat time.Time

func (timeFormat TimeFormat) MarshalJSON() ([]byte, error) {
	var stamp = fmt.Sprintf("\"%s\"", time.Time(timeFormat).Format("2006-01-02 15:04:05"))
	return []byte(stamp), nil
}

type SensorData struct {
	gorm.Model
	DevId   int        `gorm:"type: int;not null" json:"dev_id"`
	DevName string     `gorm:"type: varchar(20)" json:"dev_name"`
	Time    TimeFormat `gorm:"type: datetime" json:"time"`
	Tem     float64    `gorm:"type: double" json:"tem"`
	Hum     float64    `gorm:"type: double" json:"hum"`
	CoCon   float64    `gorm:"type: double" json:"coCon"`
	PmCon   float64    `gorm:"type: double" json:"pmCon"`
	Smk     float64    `gorm:"type: double" json:"smk"`
	Alarm   int        `gorm:"type: int" json:"alarm"`
}

func StoreSensorData(sensorData SensorData) int {
	result := db.Create(&sensorData)
	if result.RowsAffected < 1 {
		return errmsg.ERROR
	}
	return errmsg.SUCCESS
}

func GetSensorData(devName string, begin time.Time, end time.Time, pageNum int, pageSize int) ([]SensorData, int, int) {
	var dataGroup []SensorData
	db.Limit(pageSize).Offset((pageNum-1)*pageSize).Where("dev_name = ? AND time between ? and ?", devName, begin, end).Find(&dataGroup)
	var total int64
	db.Model(&SensorData{}).Where("dev_name = ? AND time between ? and ?", devName, begin, end).Count(&total)
	return dataGroup, int(total), errmsg.SUCCESS
}

func GetSensorDataTotal(devName string) (int, int) {
	var total int64
	db.Model(&SensorData{}).Where("dev_name = ?", devName).Count(&total)
	return int(total), errmsg.SUCCESS
}

func GetSensorDataDescendWithLimit(devName string, limit int) ([]SensorData, int) {
	var dataGroup []SensorData
	db.Where("dev_name = ?", devName).Order("id desc").Limit(limit).Find(&dataGroup)
	return dataGroup, errmsg.SUCCESS
}

func GetAllSensorData() ([]SensorData, int, int) {
	var dataGroup []SensorData
	result := db.Find(&dataGroup)
	return dataGroup, int(result.RowsAffected), errmsg.SUCCESS
}
