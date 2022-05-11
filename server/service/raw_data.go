package service

import (
	"gorm.io/gorm"
	"smart-home/model"
	"smart-home/sql"
	"smart-home/utils/errmsg"
)

func StoreRawData(rawData model.RawData) int {
	deviceData := model.DeviceData{
		DevId:   rawData.DevId,
		DevName: rawData.DevName,
		DevVol:  rawData.DevVol,
		DevBat:  rawData.DevBat,
		DevAng:  rawData.DevAng,
		DevSig:  rawData.DevSig,
	}
	sensorData := model.SensorData{
		DevId:   rawData.DevId,
		DevName: rawData.DevName,
		Time:    rawData.Time,
		Tem:     rawData.Tem,
		Hum:     rawData.Hum,
		CoCon:   rawData.CoCon,
		PmCon:   rawData.PmCon,
		Smk:     rawData.Smk,
		Alarm:   rawData.Alarm,
	}
	// 开启事务
	err := sql.Db.Transaction(func(tx *gorm.DB) error {
		if err := tx.Create(&deviceData).Error; err != nil {
			return err
		}
		if err := tx.Create(&sensorData).Error; err != nil {
			return err
		}
		// 提交事务
		return nil
	})
	if err != nil {
		return errmsg.ERROR
	}
	return errmsg.SUCCESS
}
